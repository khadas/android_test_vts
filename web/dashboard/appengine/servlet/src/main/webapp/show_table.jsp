<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>

<html>
  <head>
      <title>VTS Table</title>
      <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
      <script type="text/javascript">
        google.charts.load('current', {'packages':['table']});
        google.charts.setOnLoadCallback(drawGridTable);
        google.charts.setOnLoadCallback(drawProfilingTable);


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

            var table = new google.visualization.Table(document.getElementById('profiling_table_div'));
            table.draw(data,
                      {showRowNumber: true, alternatingRowStyle : true});

            google.visualization.events.addListener(table, 'select', selectHandler);

            function selectHandler(e) {
                var ctx = "${pageContext.request.contextPath}";
                var link = ctx + "/show_graph?profilingPoint=" +
                    data.getValue(table.getSelection()[0].row, 0);
                window.open(link,"_self");
            }
        }

        // table for grid data
        function drawGridTable() {
            var data = new google.visualization.DataTable();

            // add columns
            data.addColumn('string', 'Stats Type  \\ Build ID'); // blank column for first
            var buildIDtimeStampArray = ${buildIDtimeStampArrayJson};
            for (var i = 0; i < buildIDtimeStampArray.length; i++) {
                // trim the time stamp
                var colName = buildIDtimeStampArray[i].substring(0, buildIDtimeStampArray[i].lastIndexOf("."));
                data.addColumn('string', colName);
            }

            // add rows
            var finalGrid = ${finalGridJson};

            // bold the first seven rows - first column
            for (var i = 0; i < 7; i++) {
                finalGrid[i][0] = finalGrid[i][0].bold();
            }
            data.addRows(finalGrid);

            // add colors to the grid data table - iterate rowArray to view saved results.
            // first seven rows are for the summary grid and the device info grid
            for (var testCaseIndex = 7; testCaseIndex < finalGrid.length; testCaseIndex++) {  // test case name
                for (var buildIdIndex = 1; buildIdIndex < finalGrid[0].length; buildIdIndex++) {  // build ID

                    var result = finalGrid[testCaseIndex][buildIdIndex];
                    switch(result) {
                        case '0':
                            data.setProperty(testCaseIndex, buildIdIndex, 'style',
                                'background-color: #A8A8A8;'); // Case: unknown - grey
                            break;
                        case '1':
                            data.setProperty(testCaseIndex, buildIdIndex, 'style',
                                'background-color: #7FFF00;'); // Case: pass - green
                            break;
                        case '2':
                            data.setProperty(testCaseIndex, buildIdIndex, 'style',
                                'background-color: #ff4d4d;'); // Case : fail - red
                            break;
                        case '3':
                        case '4':
                        case '5':
                            data.setProperty(testCaseIndex, buildIdIndex, 'style',
                                'background-color: #ff944d;'); // Case: skip, exception, timeout - orange
                            break;
                    }
                    data.setValue(testCaseIndex, buildIdIndex, '');
                }
            }

            var table = new google.visualization.Table(document.getElementById('grid_table_div'));
            table.draw(data, {showRowNumber: false, alternatingRowStyle : true, 'allowHtml': true,
                              frozenColumns: 1});
        }
      </script>
  </head>

  <body>
    <!-- Home page logo -->
    <div id="home_page_logo_div" style="margin-left:100px; margin-top:50px">
      <a href="${pageContext.request.contextPath}/"><h2>VTS</h2>
      </a>
    </div>
    <!-- Error in case of profiling data is missing -->
    <div id="error_div" style="margin-left:100px; margin-top:50px"><h3>${error}</h3></div>
    <!-- Profiling Table -->
    <div id="profiling_table_div" style="margin-left:100px; margin-top:50px"></div>

    <!-- Grid tables-->
    <div style="margin-left:100px;margin-top:50px">
        <input id="previous_button" type="button" value="<<Previous" onclick="navigate(-1);" />
        <input id="next_button" type="button" value="Next>>" onclick="navigate(1);" />
    </div>

    <div id="grid_table_div" style="margin-left:100px"></div>

    <script type="text/javascript">
        // disable buttons on load
        var pageNumber = ${buildIdPageNo};
        var maxPageNumber = ${maxBuildIdPageNo};
        document.getElementById("previous_button").disabled = (pageNumber == 0) ? true : false;
        document.getElementById("next_button").disabled = (pageNumber == maxPageNumber) ? true : false;

        // hide the table if if error message is empty.
        var errorMessage = ${errorJson};
        if (errorMessage.length > 0) {
            document.getElementById('profiling_table_div').style.display = "none";
        }

        // for navigating grid table thorugh previous and next buttons
        function navigate(inc) {
            var pageNumber = ${buildIdPageNo};
            var maxPageNumber = ${maxBuildIdPageNo};
            var nextPage = parseInt(pageNumber) + parseInt(inc);
            nextPage = Math.max(nextPage, 0);
            nextPage = Math.min(nextPage, maxPageNumber);
            var link = "${pageContext.request.contextPath}" + "/show_table?tableName=" + ${tableName} +
                       "&" + "buildIdPageNo=" + nextPage;
            window.open(link,"_self");
        }
    </script>
  </body>
</html>