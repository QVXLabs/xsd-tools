[@lua
-- Arbitrary-precision leaf for unsignedLong: 2^64-1 overflows a signed Long,
-- so it round-trips through org.json's BigInteger accessors instead.
type_biginteger = {
   typename   = 'java.math.BigInteger',
   marshal   = 'put',
   unmarshal = 'getBigInteger',
   declrFmt = '%s',
   metaInfo = { primative = true }
}
]
