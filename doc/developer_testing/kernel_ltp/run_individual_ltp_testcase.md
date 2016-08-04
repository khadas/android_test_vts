# How to run individual LTP test case
## 1. Compile LTP source code
'make vts -j12'

__Note: the following instructions will be simplified after vts supports tradefeded run command filter)__
## 2. Copy all test binaries
'adb push out/host/linux-x86/vts/android-vts/testcases/32/ltp/. /data/local/tmp/ltp'

## 3. Setup basic environment setup such as a temp directory for LTP
'adb shell rm -rf /data/local/tmp/ltp/tmp'
'adb shell mkdir /data/local/tmp/ltp/tmp'
'adb shell chmod 775 /data/local/tmp/ltp/tmp'

## 4. Run individual LTP test case with arguments
'adb shell env TMP=/data/local/tmp/ltp/tmp PATH=/system/bin:/data/local/tmp/ltp/testcases/bin LTP_DEV_FS_TYPE=ext4 TMPDIR=/data/local/tmp/ltp/tmp LTPROOT=/data/local/tmp/ltp /data/local/tmp/ltp/testcases/bin/<binary> <args1 args2 ...>'

Test binaries and ars for test cases can be found at 'out/host/linux-x86/vts/android-vts/testcases/32/ltp/ltp_vts_testcases.txt'.

Example:test case chdir01A in testsuite syscalls' arguments are "-T chdir01":

'adb shell env TMP=/data/local/tmp/ltp/tmp PATH=/system/bin:/data/local/tmp/ltp/testcases/bin LTP_DEV_FS_TYPE=ext4 TMPDIR=/data/local/tmp/ltp/tmp LTPROOT=/data/local/tmp/ltp /data/local/tmp/ltp/testcases/bin/chdir01A -T chdir01'
