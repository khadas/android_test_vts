# How to run individual LTP test case
## 1. Compile VTS and LTP source code
'make vts -j12'

__Note: The following instructions will be simplified to a single line command after VTS supports TradeFed run command filtering__
## 2. Copy all test binaries
'adb push out/host/linux-x86/vts/android-vts/testcases/32/ltp/. /data/local/tmp/ltp'

## 3. Setup basic environment setup such as a temp directory for LTP
'adb shell rm -rf /data/local/tmp/ltp/tmp'

'adb shell mkdir /data/local/tmp/ltp/tmp'

'adb shell chmod 775 /data/local/tmp/ltp/tmp'

## 4. Enter adb shell and set environment variables
'adb shell'

'export TMP=/data/local/tmp/ltp/tmp PATH=/system/bin:/data/local/tmp/ltp/testcases/bin LTP_DEV_FS_TYPE=ext4 TMPDIR=/data/local/tmp/ltp/tmp LTPROOT=/data/local/tmp/ltp'

## 5. Find test case commands
Test case commands can be found in test definitions in file 'out/host/linux-x86/vts/android-vts/testcases/32/ltp/ltp_vts_testcases.txt'.

Each line in the file is a test definition containing three parts separated by tabs:

<test suite>\t<test name>\t<commands>

For example: for test case syscalls-move_pages03, the definition is as follows:

'syscalls   move_pages03    cd $LTPROOT/testcases/bin && chown root move_pages03 && chmod 04755 move_pages03 && move_pages.sh 03'

The command required to run the test is:

'cd $LTPROOT/testcases/bin && chown root move_pages03 && chmod 04755 move_pages03 && move_pages.sh 03'

## 6. Prepare test case commands
There are three things one can do to prepare a test command.

### 6.1 Set permission with chmod
In the syscalls-move_pages03 example above, in order to run the command one has to use chmod to set the test executable 'move_pages.sh' before running:

'chmod 755 $LTPROOT/testcases/bin/move_pages.sh'

### 6.2 Replace test case executable with absolute path
Instead of using chmod, one can also simply replace the test case executable names with their absolute path:

Replace: 'move_pages.sh' with '$LTPROOT/testcases/bin/move_pages.sh'

### 6.3 Replace semicolons with &&
Some test case definitions contain semicolons in their definition. For example, test case fs-gf12 has the following command:

'mkfifo $TMPDIR/gffifo17; growfiles -b -W gf12 -e 1 -u -i 0 -L 30 $TMPDIR/gffifo17'

Replacing ';' with '&&', the command becomes:

'mkfifo $TMPDIR/gffifo17&& growfiles -b -W gf12 -e 1 -u -i 0 -L 30 $TMPDIR/gffifo17'

## 7. Execute the test case command
Execute the test case command in adb shell.

In case of #6.1 above, the command is

'cd $LTPROOT/testcases/bin && chown root move_pages03 && chmod 04755 move_pages03 && move_pages.sh 03'

In case of #6.2 above, the command is

'cd $LTPROOT/testcases/bin && chown root move_pages03 && chmod 04755 move_pages03 && $LTPROOT/testcases/move_pages.sh 03'

In case of #6.3 above, the command is

'mkfifo $TMPDIR/gffifo17&& growfiles -b -W gf12 -e 1 -u -i 0 -L 30 $TMPDIR/gffifo17'