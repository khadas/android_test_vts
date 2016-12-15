<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>

<html>
  <head>
      <title>VTS Table</title>
      <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
      <script type="text/javascript">
        google.charts.load('current', {'packages':['table', 'corechart']});
        google.charts.setOnLoadCallback(drawGridTable);
        google.charts.setOnLoadCallback(drawProfilingTable);
        google.charts.setOnLoadCallback(drawLegendTable);
        google.charts.setOnLoadCallback(drawPieChart);

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

            var table = new google.visualization.Table(
                document.getElementById('profiling_table_div'));
            table.draw(data,
                      {showRowNumber: true, alternatingRowStyle : true});

            google.visualization.events.addListener(table, 'select', selectHandler);

            function selectHandler(e) {
                var ctx = "${pageContext.request.contextPath}";
                var link = ctx + "/show_graph?profilingPoint=" +
                    data.getValue(table.getSelection()[0].row, 0) +
                    "&tableName=" + ${tableName} +
                    "&buildIdStartTime=" + ${buildIdStartTime} +
                    "&buildIdEndTime=" + ${buildIdEndTime};
                window.open(link,"_self");
            }
        }

        // to draw pie chart
        function drawPieChart() {
            var pieChartArray = ${pieChartArrayJson};
            if (pieChartArray.length < 1) {
                return;
            }
            for (var i = 1; i < pieChartArray.length; i++) {
                pieChartArray[i][1] = parseInt(pieChartArray[i][1]);
            }

            var data = google.visualization.arrayToDataTable(pieChartArray);
            var topBuild = ${topBuildJson};
            var colName = topBuild.substring(0, topBuild.lastIndexOf("."));
            var title = 'Test Result Status for build ID : ' + colName;
            var options = {
              title: title,
              is3D: false,
              colors: ['#FFFFFF', '#7FFF00', '#ff4d4d', '#A8A8A8', '#000000', '9900CC'],
              legend: { position: 'labeled' }
            };

            var chart = new google.visualization.PieChart(document.getElementById('pie_chart_div'));
            chart.draw(data, options);
        }
        // table for legend
        function drawLegendTable() {
            var data = new google.visualization.DataTable();

            var columns = new Array('Unknown', 'Pass', 'Fail', 'Skipped', 'Exception', 'Timeout');
            // add columns
            data.addColumn('string', 'Result');
            for (var i = 0; i < columns.length; i++) {
                data.addColumn('string', columns[i]);
            }

            var row = new Array('Color Code', '-', ' ', ' ', '~', ' ', ' ');
            var rows = new  Array(1);
            rows[0] = row;
            data.addRows(rows);

            // add colors
            for (var row = 0; row < rows.length; row++) {
                for (var column = 1; column < rows[0].length; column++) {
                    var result = column - 1;
                    switch(result) {
                        case 0 : // case : unknown
                            data.setValue(row, column, '-');
                            break;
                        case 1 : // Case: pass - green
                            data.setProperty(row, column, 'style',
                                'background-color: #7FFF00;');
                            break;
                        case 2 : // Case : fail - red
                            data.setProperty(row, column, 'style',
                                'background-color: #ff4d4d;');
                            break;
                        case 3 : // case : skip : grey
                            data.setProperty(row, column, 'style',
                                'background-color: #A8A8A8;');
                            data.setValue(row, column, '~');
                            break;
                        case 4: // Case: Exception - black
                            data.setProperty(row, column, 'style',
                                'background-color: #000000;');
                            break;
                        case 5: // Case: Timeout - purple
                            data.setProperty(row, column, 'style',
                                'background-color: #9900CC;');
                            break;
                    }
                }
            }

            var table = new google.visualization.Table(document.getElementById('legend_table_div'));
            table.draw(data, {showRowNumber: false, alternatingRowStyle : true, 'allowHtml': true,
                              frozenColumns: 1});
        }

        // table for grid data
        function drawGridTable() {
            var data = new google.visualization.DataTable();

            // add columns
            data.addColumn('string', 'Stats Type  \\ Build ID'); // blank column for first
            var buildIDtimeStampArray = ${buildIDtimeStampArrayJson};
            for (var i = 0; i < buildIDtimeStampArray.length; i++) {
                // trim the time stamp
                var colName = buildIDtimeStampArray[i].substring(
                    0, buildIDtimeStampArray[i].lastIndexOf("."));
                data.addColumn('string', colName);
            }

            // add rows
            var finalGrid = ${finalGridJson};

            var summaryRowCount = ${summaryRowCountJson};
            var deviceInfoRowCount = ${deviceInfoRowCountJson};

            // bold the first few rows - first column
            for (var i = 0; i < summaryRowCount + deviceInfoRowCount; i++) {
                finalGrid[i][0] = finalGrid[i][0].bold();
            }

            // add rows to the data.
            data.addRows(finalGrid);


            // add colors to the grid data table - iterate rowArray to view saved results.
            for (var testCaseIndex = summaryRowCount + deviceInfoRowCount;
                 testCaseIndex < finalGrid.length; testCaseIndex++) {  // test case name
                for (var buildIdIndex = 1; buildIdIndex < finalGrid[0].length;
                     buildIdIndex++) {  // build ID

                    var result = finalGrid[testCaseIndex][buildIdIndex];
                    switch(result) {
                        case '0': // case : unknown
                            data.setValue(testCaseIndex, buildIdIndex, '-');
                            break;

                        case '1': // Case: pass - green
                            data.setProperty(testCaseIndex, buildIdIndex, 'style',
                                'background-color: #7FFF00;');
                            data.setValue(testCaseIndex, buildIdIndex, '');
                            break;

                        case '2': // Case : fail - red
                            data.setProperty(testCaseIndex, buildIdIndex, 'style',
                                'background-color: #ff4d4d;');
                            data.setValue(testCaseIndex, buildIdIndex, '');
                            break;

                        case '3': // Case: skip
                            data.setProperty(testCaseIndex, buildIdIndex, 'style',
                                'background-color: #A8A8A8;');
                            data.setValue(testCaseIndex, buildIdIndex, '~');
                            break;

                        case '4': // Case: Exception - black
                            data.setProperty(testCaseIndex, buildIdIndex, 'style',
                                'background-color: #000000;');
                            data.setValue(testCaseIndex, buildIdIndex, ' ');
                            break;
                        case '5': // Case: Timeout - purple
                            data.setProperty(testCaseIndex, buildIdIndex, 'style',
                                'background-color: #9900CC;');
                            data.setValue(testCaseIndex, buildIdIndex, ' ');
                            break;
                    }
                }
            }

            var table = new google.visualization.Table(document.getElementById('grid_table_div'));
            table.draw(data, {showRowNumber: false, alternatingRowStyle : true, 'allowHtml': true,
                              frozenColumns: 1});

            // selection handler for code coverage
            google.visualization.events.addListener(table, 'select', selectHandler);

            // opens up new page for code coverage
            // equivalent to setting up hyperlinks for the 8th row to link it
            // to the code coverage page.
            function selectHandler() {
                var selection = table.getSelection();
                var summaryRowCount = ${summaryRowCountJson};
                var deviceInfoRowCount = ${deviceInfoRowCountJson};

                // return if it is not the % coverage row
                // The click should be made only on the last row to see coverage data.
                var rowIndex = table.getSelection()[0].row;
                if (rowIndex != summaryRowCount + deviceInfoRowCount -1) {
                    return;
                }

                if (selection.length == 0) return;
                var cell = event.target; // get selected cell
                var column = cell.cellIndex - 1;
                var buildIDtimeStampArray = ${buildIDtimeStampArrayJson};

                // key is a unique combination of build ID and time stamp
                var key = buildIDtimeStampArray[column];

                var ctx = "${pageContext.request.contextPath}";

                var link = ctx + "/show_coverage?key=" + key + "&tableName="
                     + ${tableName};
                window.open(link,"_self");
            }
        }
      </script>
  </head>

  <body>
    <!-- pie chart -->
    <div id = "pie_chart_div" style="width: 800px; height: 400px; float:right; display:inline"></div>

    <!-- Home page logo -->
    <div id="home_page_logo_div" style="margin-left:100px; margin-top:50px">
      <a href="${pageContext.request.contextPath}/"><h2>VTS</h2>
      </a>
    </div>
    <!-- Error in case of profiling data is missing -->
    <div id="error_div" style="margin-left:100px; margin-top:50px"><h3>${error}</h3></div>
    <!-- Profiling Table -->
    <div id="profiling_table_div" style="margin-left:100px; margin-top:50px"></div>


    <div style="clear:both; margin-left:100px; overflow: hidden;">
        <div id = "legend_table_div" style="margin-top:10px"></div>

        <!-- Grid tables-->
        <div style="margin-top:10px">
            <input id="newer_button" type="button" value="<<Newer" onclick="prev();" />
            <input id="older_button" type="button" value="Older>>" onclick="next();" />
        </div>
        <div id="grid_table_div"></div>
    </div>

    <script type="text/javascript">
        // disable buttons on load
        document.getElementById("newer_button").disabled = !Boolean(${hasNewer});
        document.getElementById("older_button").disabled = !Boolean(${hasOlder});

        // hide the table if if error message is empty.
        var errorMessage = ${errorJson};
        if (errorMessage.length > 0) {
            document.getElementById('profiling_table_div').style.display = "none";
        }

        // for navigating grid table through previous and next buttons
        function next() {
            var endTime = ${buildIdStartTime};
            var link = "${pageContext.request.contextPath}" + "/show_table?tableName="
                + ${tableName} + "&buildIdEndTime=" + endTime;
            window.open(link,"_self");
        }

        function prev() {
            var startTime = ${buildIdEndTime};
            var link = "${pageContext.request.contextPath}" + "/show_table?tableName="
                + ${tableName} + "&buildIdStartTime=" + startTime;
            window.open(link,"_self");
        }
    </script>
  </body>
</html>
