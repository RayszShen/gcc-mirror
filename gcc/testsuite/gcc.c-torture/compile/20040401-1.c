int __atomic_readv_replacement(unsigned char iov_len, int count, int i) {
    unsigned char bytes = 0;
    if ((unsigned char)((char)127 - bytes) < iov_len)
      return 22;
    return 0;
}
