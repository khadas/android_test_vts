# How to run all the enabled LTP tests
## 1. Compile VTS and LTP source code
'make vts -j12'

## 2. Start VTS-Tradefed
'vts-tradefed'
'>run vts-kernel'

This will take around 30 minutes to run.
Results can found at out/host/linux-x86/vts/android-vts/results/
Device logcat and host logs will be found at: "out/host/linux-x86/vts/android-vts/logs/"
