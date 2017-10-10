#!/bin/bash
rm -f test.onekpaq test.raw
# test results only valid when using the same context cache!
../onekpaq_encode 4 3 recede1.data recede2.data recede3.data recede4.data recede5.data test.onekpaq
diff test.onekpaq recede.onekpaq || exit 1
../onekpaq_decode recede.onekpaq test.raw
diff test.raw recede.raw