# PythonVirtualenvPreparerTest

This directory tests the functionality of VtsPythonVirtualenvPreparer.


Two modules are included in this project:

* VtsSelfTestPythonVirtualenvPreparerTestPart0
* VtsSelfTestPythonVirtualenvPreparerTestPart1
* VtsSelfTestPythonVirtualenvPreparerTestPart2


VtsSelfTestPythonVirtualenvPreparerTestPart1 uses a module level
VtsPythonVirtualenvPreparer and VtsSelfTestPythonVirtualenvPreparerTestPart2
does not. The naming of `Part0`, `Part1` and `Part2` is to ensure the order
of test execution to test whether a module level VtsPythonVirtualenvPreparer's
tearDown functions will affect plan level VtsPythonVirtualenvPreparer.

