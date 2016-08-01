<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>

<html>
  <head>
      <title>VTS Table</title>
      <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
      <script type="text/javascript">
        google.charts.load('current', {'packages':['table']});
        google.charts.setOnLoadCallback(drawGridTable);
        google.charts.setOnLoadCallback(drawProfilingTable);

        // table for grid data
        function drawGridTable() {
          var data = new google.visualization.DataTable();
          var summaryTableData = new google.visualization.DataTable();

          // add columns
          data.addColumn('string', 'Test Cases \\ Build ID'); // blank column for first
          summaryTableData.addColumn('string', 'Stats Type \\ Build ID');
          var buildNameArray = ${buildNameArrayJson};
          buildNameArray.sort();
          for (var i = 0; i < buildNameArray.length; i++) {
              data.addColumn('string', buildNameArray[i]);
              summaryTableData.addColumn('string', buildNameArray[i]);
          }

          // add rows
          var testCaseNameListArray = ${testCaseNameListJson};
          var rowArray = new Array(testCaseNameListArray.length);
          var summaryTableRowArray = new Array(3);

          var testCaseResultMap = ${testCaseResultMapJson};

          var totalRow = new Array(buildNameArray.length + 1);
          var passRow = new Array(buildNameArray.length + 1);
          var ratioRow = new Array(buildNameArray.length + 1);
          for (var i = 0; i < rowArray.length; i++) {  // test case name
              var row = new Array(buildNameArray.length + 1);
              for (var j = 0; j < row.length; j++) {  // build ID
                  if (j == 0) {
                      row[j] = testCaseNameListArray[i];
                      totalRow[j] = "total";
                      passRow[j] = "#passed";
                      ratioRow[j] = "%passed";
                  } else {
                      if (i == 0) {
                          passRow[j] = 0;
                          totalRow[j] = 0;
                      }
                      var result = testCaseResultMap[buildNameArray[j] + "." + testCaseNameListArray[i]];
                      if (result == 1) {
                          row[j] = 'O';
                          passRow[j] += 1;
                      } else if (result == 2) row[j] = 'X';
                      else if (result == 0) row[j] = '?';
                      else row[j] = '-';
                      totalRow[j] += 1;
                      if (i == rowArray.length - 1) {
                          ratioRow[j] = "" + Math.round(passRow[j] * 1000 / totalRow[j]) / 10 + "%";
                          passRow[j] = "" + passRow[j];
                          totalRow[j] = "" + totalRow[j];
                      }
                  }
              }
              rowArray[i] = row;
          }
          data.addRows(rowArray);
          summaryTableRowArray[0] = totalRow;
          summaryTableRowArray[1] = passRow;
          summaryTableRowArray[2] = ratioRow;
          summaryTableData.addRows(summaryTableRowArray);

          // custom color to table
          var cssClassNames = {
              'headerRow': 'italic-darkblue-font large-font bold-font',
              'tableRow': 'beige-background',
              'oddTableRow': 'beige-background',
              'selectedTableRow': 'orange-background large-font',
              'hoverTableRow': '',
              'headerCell': 'gold-border',
              'tableCell': '',
              'rowNumberCell': 'underline-blue-font'};

          var summaryTable = new google.visualization.Table(document.getElementById('grid_summary_table_div'));
          summaryTable.draw(summaryTableData,
                            {showRowNumber: false, alternatingRowStyle : true,
                             frozenColumns: 1, page: 'enable', 'cssClassNames': cssClassNames});

          var table = new google.visualization.Table(document.getElementById('grid_table_div'));
          table.draw(data, {showRowNumber: false, alternatingRowStyle : true,
                            frozenColumns: 1, page: 'enable', 'cssClassNames': cssClassNames});
        }

        // table for profiling data
        function drawProfilingTable() {

          var data = new google.visualization.DataTable();

          // add columns
          data.addColumn('string', 'Profiling Point Names');

          // add rows
          var profilingPointNameArray = ${profilingPointNameJson};
          var rowArray = new Array(profilingPointNameArray.length);

          for (var i = 0; i < rowArray.length; i++) {
              var row = new Array(1);
              row[0] = profilingPointNameArray[i];
              rowArray[i] = row;
          }
          data.addRows(rowArray);

          // custom color to table
          var cssClassNames = {
              'headerRow': 'italic-darkblue-font large-font bold-font',
              'tableRow': 'beige-background',
              'oddTableRow': 'beige-background',
              'selectedTableRow': 'orange-background large-font',
              'hoverTableRow': '',
              'headerCell': 'gold-border',
              'tableCell': '',
              'rowNumberCell': 'underline-blue-font'};

          var table = new google.visualization.Table(document.getElementById('profiling_table_div'));
          table.draw(data,
                    {showRowNumber: true, alternatingRowStyle : true, 'cssClassNames': cssClassNames});

          google.visualization.events.addListener(table, 'select', selectHandler);

          function selectHandler(e) {
              var ctx = "${pageContext.request.contextPath}";
              var link = ctx + "/show_graph?profilingPoint=" + data.getValue(table.getSelection()[0].row, 0);
              window.open(link,"_self");
          }
        }


      </script>
  </head>

  <body>
    <!-- Error in case of profiling data is missing -->
    <h3>${error}</h3>
    <div id="profiling_table_div" style="margin-left:100px; margin-top:50px"></div>
    <div id="grid_summary_table_div" style="margin-left:100px; margin-top:50px"></div>
    <div id="grid_table_div" style="margin-left:100px; margin-top:50px"></div>
  </body>
</html>
