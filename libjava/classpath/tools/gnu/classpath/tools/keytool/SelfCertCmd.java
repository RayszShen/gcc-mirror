/* SelfCertCmd.java -- The selfcert command handler of the keytool
   Copyright (C) 2006 Free Software Foundation, Inc.

This file is part of GNU Classpath.

GNU Classpath is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Classpath is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Classpath; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA.

Linking this library statically or dynamically with other modules is
making a combined work based on this library.  Thus, the terms and
conditions of the GNU General Public License cover the whole
combination.

As a special exception, the copyright holders of this library give you
permission to link this library with independent modules to produce an
executable, regardless of the license terms of these independent
modules, and to copy and distribute the resulting executable under
terms of your choice, provided that you also meet, for each linked
independent module, the terms and conditions of the license of that
module.  An independent module is a module which is not derived from
or based on this library.  If you modify this library, you may extend
this exception to your version of the library, but you are not
obligated to do so.  If you do not wish to do so, delete this
exception statement from your version. */


package gnu.classpath.tools.keytool;

import gnu.java.security.x509.X500DistinguishedName;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.security.InvalidKeyException;
import java.security.Key;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.SignatureException;
import java.security.UnrecoverableKeyException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.logging.Logger;

import javax.security.auth.callback.UnsupportedCallbackException;
import javax.security.auth.x500.X500Principal;

/**
 * The <b>-selfcert</b> keytool command handler is used to generate a self-
 * signed X.509 version 1 certificate using key store credentials stored under a
 * designated alias.
 * <p>
 * Possible options for this command are:
 * <p>
 * <dl>
 *      <dt>-alias ALIAS</dt>
 *      <dd>Every entry, be it a <i>Key Entry</i> or a <i>Trusted
 *      Certificate</i>, in a key store is uniquely identified by a user-defined
 *      <i>Alias</i> string. Use this option to specify the <i>Alias</i> to use
 *      when referring to an entry in the key store. Unless specified otherwise,
 *      a default value of <code>mykey</code> shall be used when this option is
 *      omitted from the command line.
 *      <p></dd>
 *      
 *      <dt>-sigalg ALGORITHM</dt>
 *      <dd>The canonical name of the digital signature algorithm to use for
 *      signing the certificate. If this option is omitted, a default value will
 *      be chosen based on the type of the private key associated with the
 *      designated <i>Alias</i>. If the private key is a <code>DSA</code> one,
 *      the value for the signature algorithm will be <code>SHA1withDSA</code>.
 *      If on the other hand the private key is an <code>RSA</code> one, then
 *      the tool will use <code>MD5withRSA</code> as the signature algorithm.
 *      <p></dd>
 *      
 *      <dt>-dname NAME</dt>
 *      <dd>Use this option to specify the <i>Distinguished Name</i> of the
 *      newly generated self-signed certificate. If this option is omitted, the
 *      existing <i>Distinguished Name</i> of the base certificate in the chain
 *      associated with the designated <i>Alias</i> will be used instead.
 *      <p>
 *      The syntax of a valid value for this option MUST follow RFC-2253
 *      specifications. Namely the following components (with their accepted
 *      meaning) will be recognized. Note that the component name is case-
 *      insensitive:
 *      <dl>
 *              <dt>CN</dt>
 *              <dd>The Common Name; e.g. "host.domain.com"</dd>
 *              
 *              <dt>OU</dt>
 *              <dd>The Organizational Unit; e.g. "IT Department"</dd>
 *              
 *              <dt>O</dt>
 *              <dd>The Organization Name; e.g. "The Sample Company"</dd>
 *              
 *              <dt>L</dt>
 *              <dd>The Locality Name; e.g. "Sydney"</dd>
 *              
 *              <dt>ST</dt>
 *              <dd>The State Name; e.g. "New South Wales"</dd>
 *              
 *              <dt>C</dt>
 *              <dd>The 2-letter Country identifier; e.g. "AU"</dd>
 *      </dl>
 *      <p>
 *      When specified with a <code>-dname</code> option, each pair of component
 *      / value will be separated from the other with a comma. Each component
 *      and value pair MUST be separated by an equal sign. For example, the
 *      following is a valid DN value:
 *      <pre>
 *        CN=host.domain.com, O=The Sample Company, L=Sydney, ST=NSW, C=AU
 *      </pre>
 *      <p></dd>
 *      
 *      <dt>-validity DAY_COUNT</dt>
 *      
 *      <dt>-keypass PASSWORD</dt>
 *      
 *      <dt>-storetype STORE_TYP}</dt>
 *      <dd>Use this option to specify the type of the key store to use. The
 *      default value, if this option is omitted, is that of the property
 *      <code>keystore.type</code> in the security properties file, which is
 *      obtained by invoking the {@link java.security.KeyStore#getDefaultType()}
 *      static method.
 *      <p></dd>
 *      
 *      <dt>-keystore URL</dt>
 *      <dd>Use this option to specify the location of the key store to use.
 *      The default value is a file {@link java.net.URL} referencing the file
 *      named <code>.keystore</code> located in the path returned by the call to
 *      {@link java.lang.System#getProperty(String)} using <code>user.home</code>
 *      as argument.
 *      <p>
 *      If a URL was specified, but was found to be malformed --e.g. missing
 *      protocol element-- the tool will attempt to use the URL value as a file-
 *      name (with absolute or relative path-name) of a key store --as if the
 *      protocol was <code>file:</code>.
 *      <p></dd>
 *      
 *      <dt>-storepass PASSWORD</dt>
 *      <dd>Use this option to specify the password protecting the key store. If
 *      this option is omitted from the command line, you will be prompted to
 *      provide a password.
 *      <p></dd>
 *      
 *      <dt>-provider PROVIDER_CLASS_NAME</dt>
 *      <dd>A fully qualified class name of a Security Provider to add to the
 *      current list of Security Providers already installed in the JVM in-use.
 *      If a provider class is specified with this option, and was successfully
 *      added to the runtime --i.e. it was not already installed-- then the tool
 *      will attempt to removed this Security Provider before exiting.
 *      <p></dd>
 *      
 *      <dt>-v</dt>
 *      <dd>Use this option to enable more verbose output.</dd>
 * </dl>
 */
class SelfCertCmd extends Command
{
  private static final Logger log = Logger.getLogger(SelfCertCmd.class.getName());
  private String _alias;
  private String _sigAlgorithm;
  private String _dName;
  private String _password;
  private String _validityStr;
  private String _ksType;
  private String _ksURL;
  private String _ksPassword;
  private String _providerClassName;
  private X500DistinguishedName distinguishedName;
  private int validityInDays;

  // default 0-arguments constructor

  // public setters -----------------------------------------------------------

  /** @param alias the alias to use. */
  public void setAlias(String alias)
  {
    this._alias = alias;
  }

  /**
   * @param algorithm the canonical name of the digital signature algorithm to
   *          use.
   */
  public void setSigalg(String algorithm)
  {
    this._sigAlgorithm = algorithm;
  }

  /**
   * @param name the distiniguished name of both the issuer and subject (since
   *          we are dealing with a self-signed certificate) to use.
   */
  public void setDname(String name)
  {
    this._dName = name;
  }

  /**
   * @param days the string representation of the number of days (a decimal,
   *          positive integer) to assign to the generated (self-signed)
   *          certificate.
   */
  public void setValidity(String days)
  {
    this._validityStr = days;
  }

  /** @param password the (private) key password to use. */
  public void setKeypass(String password)
  {
    this._password = password;
  }

  /** @param type the key-store type to use. */
  public void setStoretype(String type)
  {
    this._ksType = type;
  }

  /** @param url the key-store URL to use. */
  public void setKeystore(String url)
  {
    this._ksURL = url;
  }

  /** @param password the key-store password to use. */
  public void setStorepass(String password)
  {
    this._ksPassword = password;
  }

  /** @param className a security provider fully qualified class name to use. */
  public void setProvider(String className)
  {
    this._providerClassName = className;
  }

  // life-cycle methods -------------------------------------------------------

  int processArgs(String[] args, int i)
  {
    int limit = args.length;
    String opt;
    while (++i < limit)
      {
        opt = args[i];
        log.finest("args[" + i + "]=" + opt);
        if (opt == null || opt.length() == 0)
          continue;

        if ("-alias".equals(opt)) // -alias ALIAS
          _alias = args[++i];
        else if ("-sigalg".equals(opt)) // -sigalg ALGORITHM
          _sigAlgorithm = args[++i];
        else if ("-dname".equals(opt)) // -dname NAME
          _dName = args[++i];
        else if ("-keypass".equals(opt)) // -keypass PASSWORD
          _password = args[++i];
        else if ("-validity".equals(opt)) // -validity DAY_COUNT
          _validityStr = args[++i];
        else if ("-storetype".equals(opt)) // -storetype STORE_TYPE
          _ksType = args[++i];
        else if ("-keystore".equals(opt)) // -keystore URL
          _ksURL = args[++i];
        else if ("-storepass".equals(opt)) // -storepass PASSWORD
          _ksPassword = args[++i];
        else if ("-provider".equals(opt)) // -provider PROVIDER_CLASS_NAME
          _providerClassName = args[++i];
        else if ("-v".equals(opt))
          verbose = true;
        else
          break;
      }

    return i;
  }

  void setup() throws Exception
  {
    setKeyStoreParams(_providerClassName, _ksType, _ksPassword, _ksURL);
    setAliasParam(_alias);
    setKeyPasswordNoPrompt(_password);
//    setDName(_dName);
    setValidityParam(_validityStr);
//    setSignatureAlgorithm(_sigAlgorithm);

    log.finer("-selfcert handler will use the following options:");
    log.finer("  -alias=" + alias);
    log.finer("  -sigalg=" + _sigAlgorithm);
    log.finer("  -dname=" + _dName);
    log.finer("  -keypass=" + _password);
    log.finer("  -validity=" + validityInDays);
    log.finer("  -storetype=" + storeType);
    log.finer("  -keystore=" + storeURL);
    log.finer("  -storepass=" + String.valueOf(storePasswordChars));
    log.finer("  -provider=" + provider);
    log.finer("  -v=" + verbose);
  }

  void start() throws KeyStoreException, NoSuchAlgorithmException,
      UnrecoverableKeyException, IOException, UnsupportedCallbackException,
      InvalidKeyException, SignatureException, CertificateException
  {
    log.entering(getClass().getName(), "start");

    // 1. get the key entry and certificate chain associated to alias
    Key privateKey = getAliasPrivateKey();
    Certificate[] chain = store.getCertificateChain(alias);

    // 2. if the user has not supplied a DN use one from the certificate chain
    X509Certificate bottomCertificate = (X509Certificate) chain[0];
    X500Principal defaultPrincipal = bottomCertificate.getIssuerX500Principal();
    setDName(_dName, defaultPrincipal);

    // 4. get alias's public key from certificate's SubjectPublicKeyInfo
    PublicKey publicKey = bottomCertificate.getPublicKey();

    // 5. issue the self-signed certificate
    setSignatureAlgorithmParam(_sigAlgorithm, privateKey);

    byte[] derBytes = getSelfSignedCertificate(distinguishedName,
                                               publicKey,
                                               (PrivateKey) privateKey);
    CertificateFactory x509Factory = CertificateFactory.getInstance("X.509");
    ByteArrayInputStream bais = new ByteArrayInputStream(derBytes);
    Certificate certificate = x509Factory.generateCertificate(bais);

    // 6. store it, w/ its private key, associating them to alias
    chain = new Certificate[] { certificate };
    store.setKeyEntry(alias, privateKey, keyPasswordChars, chain);

    // 7. persist the key store
    saveKeyStore();

    log.exiting(getClass().getName(), "start");
  }

  // own methods --------------------------------------------------------------

  private void setDName(String name, X500Principal defaultName)
  {
    if (name != null && name.trim().length() > 0)
      name = name.trim();
    else
      {
        // If dname is supplied at the command line, it is used as the X.500
        // Distinguished Name for both the issuer and subject of the certificate.
        // Otherwise, the X.500 Distinguished Name associated with alias (at the
        // bottom of its existing certificate chain) is used.
        name = defaultName.toString().trim();
      }

    distinguishedName = new X500DistinguishedName(name);
  }
}
