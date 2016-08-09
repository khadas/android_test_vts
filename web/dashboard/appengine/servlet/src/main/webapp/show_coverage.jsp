<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>


<html>
    <head>
        <title>Coverage Information</title>
        <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
        <script type="text/javascript">
            google.charts.load("current", {packages:["corechart", "table"]});
            google.charts.setOnLoadCallback(drawCoverageTable);

            // table for code coverage data
            function drawCoverageTable() {

                var data = new google.visualization.DataTable();

                // add columns
                var columns = ${coverageGridColumnsJson};
                for (var i = 0; i < columns.length; i++) {
                    data.addColumn('string', columns[i]);
                }

                // add rows
                var coverageGrid = ${coverageGridJson};
                data.addRows(coverageGrid);

                var table = new google.visualization.Table(document.getElementById('coverage_grid_div'));
                table.draw(data,
                          {showRowNumber: true, alternatingRowStyle : true, 'allowHtml': true});
            }
        </script>
    </head>
    <body>
        <div id="coverage_grid_div" style="margin-left:200px;margin-top:50px"></div>
    </body>
</html>
