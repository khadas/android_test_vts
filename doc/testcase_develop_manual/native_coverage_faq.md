# Native Coverage Information and FAQ

## 1. Code Coverage Output

### 1.1. Backgroud

To measure coverage, the source file is divided into units called basic
blocks, which may contain one or more lines of code. All code in the same basic
block are accounted for together. Some lines of code (i.e. variable
declarations) are not executable and thus belong to no basic block. Some lines
of code actually compile to several executable instructions (i.e. shorthand
conditional operators) and belong to more than one basic block.

### 1.2. Output Format

The generated coverage report displays a color-coded source file with numerical
annotations on the left margin. The row fill indicates whether or not a line of
code was executed when the tests were run: green means it was covered, red means
it was not. The corresponding numbers on the left margin indicate the number of
times the line was executed.

Lines of code that are not colored and have no execution count in the margin are
not executable instructions.

### 1.3. Example

![Screenshot of Native Code Coverage](https://screenshot.googleplex.com/fwmPDBPRO8m)

## 2. Frequently Asked Questions

### 2.1. Why do some lines have no coverage information?

The line of code is not an executable instruction. In the example below, six
lines of code are unaccounted for as they are not executable instructions:

![Example of Unexecuted Lines of Code](https://screenshot.googleplex.com/LnLjOkahqUi)

### 2.2. Why are some lines called more than expected?

Since some lines of code may belong to more than one basic block, they may
appear to have been executed more than expected. In the example below, inline
conditional notation causes line 70 to be accounted for by two basic blocks:
one repesenting the control flow, and the other representing the return.

![Example of Overlapping Accounting](https://screenshot.googleplex.com/CNuZDniHOuW)
