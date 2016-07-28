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

          // add columns
          data.addColumn('string', 'Test Cases/Build ID'); // blank column for first
          var buildNameArray = ${buildNameArrayJson};
          buildNameArray.sort();
          for (var i = 0; i < buildNameArray.length; i++) {
              data.addColumn('string', buildNameArray[i]);
          }

          // add rows
          var testCaseReportMessagesArray = ${testCaseReportMessagesJson};
          var rowArray = new Array(testCaseReportMessagesArray.length);

          for (var i = 0; i < rowArray.length; i++) {
              var row = new Array(buildNameArray.length + 1);
              for (var j = 0; j < row.length; j++) {
                  if (j == 0) {
                      row[j] = testCaseReportMessagesArray[i];
                  } else {
                      row[j] = '';
                  }
              }
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

          var table = new google.visualization.Table(document.getElementById('grid_table_div'));
          table.draw(data, {showRowNumber: false, alternatingRowStyle : true, frozenColumns: 1, page: 'enable', 'cssClassNames': cssClassNames});
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
    <div id="grid_table_div" style="margin-left:100px; margin-top:50px"></div>
  </body>
</html>
