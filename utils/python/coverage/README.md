# To run from command line
Run the parser from the command line by calling the function as follows:

  $ PYTHONPATH=$PYTHONPATH:.. python -m vts.utils.python.coverage.GCNO -f <file-name>

The output will be a printout of the lines in each block in each function described by the
specified .gcno file.


# To run from another Python module

Import the GCNO module by including the line:

   from vts.utils.python.coverage import GCNO

Run the code by calling the parse function as follows:
   summary = GCNO.parse(<file-name>)

The return value is a data structure containing a description of all functions,
blocks, arcs, and line mappings described by the specified .gcno file.
