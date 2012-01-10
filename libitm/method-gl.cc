/* Copyright (C) 2011 Free Software Foundation, Inc.
   Contributed by Torvald Riegel <triegel@redhat.com>.

   This file is part of the GNU Transactional Memory Library (libitm).

   Libitm is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Libitm is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

#include "libitm_i.h"

using namespace GTM;

namespace {

// This group consists of all TM methods that synchronize via just a single
// global lock (or ownership record).
struct gl_mg : public method_group
{
  static const gtm_word LOCK_BIT = (~(gtm_word)0 >> 1) + 1;
  // We can't use the full bitrange because ~0 in gtm_thread::shared_state has
  // special meaning.
  static const gtm_word VERSION_MAX = (~(gtm_word)0 >> 1) - 1;
  static bool is_locked(gtm_word l) { return l & LOCK_BIT; }
  static gtm_word set_locked(gtm_word l) { return l | LOCK_BIT; }
  static gtm_word clear_locked(gtm_word l) { return l & ~LOCK_BIT; }

  // The global ownership record.
  gtm_word orec;
  virtual void init()
  {
    orec = 0;
  }
  virtual void fini() { }
};

static gl_mg o_gl_mg;


// The global lock, write-through TM method.
// Acquires the orec eagerly before the first write, and then writes through.
// Reads abort if the global orec's version number changed or if it is locked.
// Currently, writes require undo-logging to prevent deadlock between the
// serial lock and the global orec (writer txn acquires orec, reader txn
// upgrades to serial and waits for all other txns, writer tries to upgrade to
// serial too but cannot, writer cannot abort either, deadlock). We could
// avoid this if the serial lock would allow us to prevent other threads from
// going to serial mode, but this probably is too much additional complexity
// just to optimize this TM method.
// gtm_thread::shared_state is used to store a transaction's current
// snapshot time (or commit time). The serial lock uses ~0 for inactive
// transactions and 0 for active ones. Thus, we always have a meaningful
// timestamp in shared_state that can be used to implement quiescence-based
// privatization safety. This even holds if a writing transaction has the
// lock bit set in its shared_state because this is fine for both the serial
// lock (the value will be smaller than ~0) and privatization safety (we
// validate that no other update transaction comitted before we acquired the
// orec, so we have the most recent timestamp and no other transaction can
// commit until we have committed).
// However, we therefore cannot use this method for a serial transaction
// (because shared_state needs to remain at ~0) and we have to be careful
// when switching to serial mode (see the special handling in trycommit() and
// rollback()).
// ??? This sharing adds some complexity wrt. serial mode. Just use a separate
// state variable?
class gl_wt_dispatch : public abi_dispatch
{
protected:
  static void pre_write(const void *addr, size_t len)
  {
    gtm_thread *tx = gtm_thr();
    if (unlikely(!gl_mg::is_locked(tx->shared_state)))
      {
	// Check for and handle version number overflow.
	if (unlikely(tx->shared_state >= gl_mg::VERSION_MAX))
	  tx->restart(RESTART_INIT_METHOD_GROUP);

	// CAS global orec from our snapshot time to the locked state.
	// This validates that we have a consistent snapshot, which is also
	// for making privatization safety work (see the class' comments).
	gtm_word now = o_gl_mg.orec;
	if (now != tx->shared_state)
	  tx->restart(RESTART_VALIDATE_WRITE);
	if (__sync_val_compare_and_swap(&o_gl_mg.orec, now,
	    gl_mg::set_locked(now)) != now)
	  tx->restart(RESTART_LOCKED_WRITE);

	// Set shared_state to new value. The CAS is a full barrier, so the
	// acquisition of the global orec is visible before this store here,
	// and the store will not be visible before earlier data loads, which
	// is required to correctly ensure privatization safety (see
	// begin_and_restart() and release_orec() for further comments).
	tx->shared_state = gl_mg::set_locked(now);
      }

    // TODO Ensure that this gets inlined: Use internal log interface and LTO.
    GTM_LB(addr, len);
  }

  static void validate()
  {
    // Check that snapshot is consistent. The barrier ensures that this
    // happens after previous data loads.
    atomic_read_barrier();
    gtm_thread *tx = gtm_thr();
    gtm_word l = o_gl_mg.orec;
    if (l != tx->shared_state)
      tx->restart(RESTART_VALIDATE_READ);
  }

  template <typename V> static V load(const V* addr, ls_modifier mod)
  {
    // Read-for-write should be unlikely, but we need to handle it or will
    // break later WaW optimizations.
    if (unlikely(mod == RfW))
      {
	pre_write(addr, sizeof(V));
	return *addr;
      }
    V v = *addr;
    if (likely(mod != RaW))
      validate();
    return v;
  }

  template <typename V> static void store(V* addr, const V value,
      ls_modifier mod)
  {
    if (unlikely(mod != WaW))
      pre_write(addr, sizeof(V));
    *addr = value;
  }

public:
  static void memtransfer_static(void *dst, const void* src, size_t size,
      bool may_overlap, ls_modifier dst_mod, ls_modifier src_mod)
  {
    if ((dst_mod != WaW && src_mod != RaW)
	&& (dst_mod != NONTXNAL || src_mod == RfW))
      pre_write(dst, size);

    if (!may_overlap)
      ::memcpy(dst, src, size);
    else
      ::memmove(dst, src, size);

    if (src_mod != RfW && src_mod != RaW && src_mod != NONTXNAL
	&& dst_mod != WaW)
      validate();
  }

  static void memset_static(void *dst, int c, size_t size, ls_modifier mod)
  {
    if (mod != WaW)
      pre_write(dst, size);
    ::memset(dst, c, size);
  }

  virtual gtm_restart_reason begin_or_restart()
  {
    // We don't need to do anything for nested transactions.
    gtm_thread *tx = gtm_thr();
    if (tx->parent_txns.size() > 0)
      return NO_RESTART;

    // Spin until global orec is not locked.
    // TODO This is not necessary if there are no pure loads (check txn props).
    gtm_word v;
    unsigned i = 0;
    while (gl_mg::is_locked(v = o_gl_mg.orec))
      {
	// TODO need method-specific max spin count
	if (++i > gtm_spin_count_var) return RESTART_VALIDATE_READ;
	cpu_relax();
      }
    // This barrier ensures that we have read the global orec before later
    // data loads.
    atomic_read_barrier();

    // Everything is okay, we have a snapshot time.
    // We don't need to enforce any ordering for the following store. There
    // are no earlier data loads in this transaction, so the store cannot
    // become visible before those (which could lead to the violation of
    // privatization safety). The store can become visible after later loads
    // but this does not matter because the previous value will have been
    // smaller or equal (the serial lock will set shared_state to zero when
    // marking the transaction as active, and restarts enforce immediate
    // visibility of a smaller or equal value with a barrier (see
    // release_orec()).
    tx->shared_state = v;
    return NO_RESTART;
  }

  virtual bool trycommit(gtm_word& priv_time)
  {
    gtm_thread* tx = gtm_thr();
    gtm_word v = tx->shared_state;

    // Special case: If shared_state is ~0, then we have acquired the
    // serial lock (tx->state is not updated yet). In this case, the previous
    // value isn't available anymore, so grab it from the global lock, which
    // must have a meaningful value because no other transactions are active
    // anymore. In particular, if it is locked, then we are an update
    // transaction, which is all we care about for commit.
    if (v == ~(typeof v)0)
      v = o_gl_mg.orec;

    // Release the orec but do not reset shared_state, which will be modified
    // by the serial lock right after our commit anyway. Also, resetting
    // shared state here would interfere with the serial lock's use of this
    // location.
    if (gl_mg::is_locked(v))
      {
	// Release the global orec, increasing its version number / timestamp.
	// TODO replace with C++0x-style atomics (a release in this case)
	atomic_write_barrier();
	v = gl_mg::clear_locked(v) + 1;
	o_gl_mg.orec = v;

	// Need to ensure privatization safety. Every other transaction must
	// have a snapshot time that is at least as high as our commit time
	// (i.e., our commit must be visible to them).
	priv_time = v;
      }
    return true;
  }

  virtual void rollback(gtm_transaction_cp *cp)
  {
    // We don't do anything for rollbacks of nested transactions.
    if (cp != 0)
      return;

    gtm_thread *tx = gtm_thr();
    gtm_word v = tx->shared_state;
    // Special case: If shared_state is ~0, then we have acquired the
    // serial lock (tx->state is not updated yet). In this case, the previous
    // value isn't available anymore, so grab it from the global lock, which
    // must have a meaningful value because no other transactions are active
    // anymore. In particular, if it is locked, then we are an update
    // transaction, which is all we care about for rollback.
    if (v == ~(typeof v)0)
      v = o_gl_mg.orec;

    // Release lock and increment version number to prevent dirty reads.
    // Also reset shared state here, so that begin_or_restart() can expect a
    // value that is correct wrt. privatization safety.
    if (gl_mg::is_locked(v))
      {
	// Release the global orec, increasing its version number / timestamp.
	// TODO replace with C++0x-style atomics (a release in this case)
	atomic_write_barrier();
	v = gl_mg::clear_locked(v) + 1;
	o_gl_mg.orec = v;

	// Also reset the timestamp published via shared_state.
	// Special case: Only do this if we are not a serial transaction
	// because otherwise, we would interfere with the serial lock.
	if (tx->shared_state != ~(typeof tx->shared_state)0)
	  tx->shared_state = v;

	// We need a store-load barrier after this store to prevent it
	// from becoming visible after later data loads because the
	// previous value of shared_state has been higher than the actual
	// snapshot time (the lock bit had been set), which could break
	// privatization safety. We do not need a barrier before this
	// store (see pre_write() for an explanation).
	__sync_synchronize();
      }

  }

  CREATE_DISPATCH_METHODS(virtual, )
  CREATE_DISPATCH_METHODS_MEM()

  gl_wt_dispatch() : abi_dispatch(false, true, false, false, &o_gl_mg)
  { }
};

} // anon namespace

static const gl_wt_dispatch o_gl_wt_dispatch;

abi_dispatch *
GTM::dispatch_gl_wt ()
{
  return const_cast<gl_wt_dispatch *>(&o_gl_wt_dispatch);
}
