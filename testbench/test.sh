#!/bin/bash
rm -f test.onekpaq test.raw
# test results only valid when using the same context cache!
../onekpaq_encode 4 3 recede1.data recede2.data recede3.data recede4.data recede5.data test.onekpaq
../onekpaq_decode test.onekpaq test.raw
diff test.raw recede.raw
rm -f test.onekpaq test.raw
