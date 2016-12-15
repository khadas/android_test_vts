<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>


<html>
<head>
    <title>Graph</title>
    <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.1.0/jquery.min.js"></script>
    <link rel="stylesheet" href="https://ajax.googleapis.com/ajax/libs/jqueryui/1.11.4/themes/smoothness/jquery-ui.css">
    <script src="https://ajax.googleapis.com/ajax/libs/jqueryui/1.11.4/jquery-ui.min.js"></script>
    <script type="text/javascript">
        google.charts.load("current", {packages:["corechart", "table", "line"]});
        google.charts.setOnLoadCallback(drawProfilingChart);
        google.charts.setOnLoadCallback(drawPerformanceChart);
        google.charts.setOnLoadCallback(drawPercentileTable);

        ONE_DAY = 86400000000000;
        NANO_PER_MILLI = 1000000;

        $(function() {
            var startTime = ${startTime};
            var endTime = ${endTime};
            if (!startTime || !endTime) {
                var now = (new Date()).getTime()*NANO_PER_MILLI;
                startTime = now - ONE_DAY;
                endTime = now;
            }
            var fromDate = new Date(startTime/NANO_PER_MILLI);
            var toDate = new Date(endTime/NANO_PER_MILLI);
            var from = $("#from").datepicker({
                    showAnim: "slideDown",
                    defaultDate: fromDate,
                    changeMonth: true
                  }).on("change", function() {
                      to.datepicker("option", "minDate", getDate(this));
                  }),
                to = $("#to").datepicker({
                    showAnim: "slideDown",
                    defaultDate: toDate,
                    changeMonth: true
                  }).on("change", function() {
                      from.datepicker("option", "maxDate", getDate(this));
                  });

            function getDate(element) {
              var date;
              try {
                date = $.datepicker.parseDate("mm/dd/yy", element.value);
              } catch( error ) {
                date = null;
              }
              return date;
            }

            from.datepicker("setDate", fromDate);
            from.datepicker("option", "maxDate", toDate);
            to.datepicker("setDate", toDate);
            to.datepicker("option", "minDate", fromDate);
        });

        function drawProfilingChart() {

            var showProfilingGraph = ${showProfilingGraph};
            if (!showProfilingGraph) {
                document.getElementById('profiling_chart_div').style.display = 'none';
                return;
            }

            var valuesArray = ${performanceValuesJson};
            var histogramData = new Array(valuesArray.length);
            for (var i = 0; i < histogramData.length; i++) {
                histogramData[i] = new Array(2);
                histogramData[i][0] = '';
                histogramData[i][1] = valuesArray[i];
            }
            var min = Math.min.apply(null, valuesArray),
                max = Math.max.apply(null, valuesArray);

            var histogramTicks = new Array(10);
            var delta = (max - min)/10;
            for (var i = 0; i <= 10; i++) {
                histogramTicks[i] = Math.round(min + delta * i);
            }

            var data = google.visualization.arrayToDataTable(histogramData, true);

            var options = {
              title: 'Approximating Normal Distribution',
              legend: { position: 'none' },
              colors: ['#4285F4'],
              vAxis:{
                title:"Frequency"
              },
              hAxis: {
                ticks: histogramTicks,
                title:"Time Taken (milliseconds)"
              },
              bar: { gap: 0 },

              histogram: {
                bucketSize: 0.02,
                maxNumBuckets: 200,
                minValue: min,
                maxValue: max
              }
            };
            var chart = new google.visualization.Histogram(document.getElementById('profiling_chart_div'));
            chart.draw(data, options);
        }

        function drawPerformanceChart() {
            var lineGraphValues = ${lineGraphValuesJson};
            var labelsList = ${labelsListJson};
            var profilingBuildIds = ${profilingBuildIdsJson};
            if (!labelsList || labelsList.length < 1) {
                return;
            }
            labelsList.forEach(function (label, i) {
                lineGraphValues[i].unshift(label);
            });

            var data = new google.visualization.DataTable();
            data.addColumn('string', 'Label');
            profilingBuildIds.forEach(function(build) {
                data.addColumn('number', build);
            });
            data.addRows(lineGraphValues);

            var options = {
              chart: {title: 'Performance'},
              legend: { position: 'none' },
              axes: {
                x: {
                  Labels: {label: 'Labels'}
                },
                y: {
                  Values: {label: 'Values'}
                }
              }
            };
            var chart = new google.charts.Line(document.getElementById('performance_chart_div'));
            chart.draw(data, options);
        }

        // table for profiling data
        function drawPercentileTable() {
            var showPercentileTable = ${showPercentileTable};
            if (!showPercentileTable) {
                return;
            }

            var data = new google.visualization.DataTable();

            // add columns
            var columns = ["10", "25", "50", "75", "80", "90", "95", "99"];
            for (var i = 0; i < columns.length; i++) {
                data.addColumn('string', columns[i]);
            }

            // add rows
            var percentileValues = ${percentileValuesJson};
            for (var i = 0; i < percentileValues.length; i++) {
                percentileValues[i] = percentileValues[i].toString();
            }
            var rows = new Array(1);
            rows[0] = percentileValues;
            data.addRows(rows);

            var table = new google.visualization.Table(document.getElementById('percentile_table_div'));
            table.draw(data,
                      {title: 'Percentile Values',
                       alternatingRowStyle : true});
        }
    </script>
</head>

<body>
    <div style="width: 80%; margin-left:10%">
      <h2>Profiling Point Name : ${profilingPointName}</h2>
      <div style="display:inline-block; margin-bottom: 30px">
          <input type="text" id="from" name="from">
          <label for="to"> - </label>
          <input type="text" id="to" name="to">
          <button id="load" style="margin-left:5px">Load</button>
      </div>
      <button id="b" style="float:right">Download Raw Data</button>
      <!-- Error in case of profiling data is missing -->
      <h3>${error}</h3>
    </div>

    <!-- Profiling chart for profiling values. -->
    <div id="profiling_chart_div" style="width: 80%; height: 500px;"></div>

    <!-- Percentile table -->
    <div id="percentile_table_div" style="margin-left:10%; margin-top:-40px; margin-bottom:125px"></div>

    <!-- Performance chart for label vs values. -->
    <div id="performance_chart_div" style="width:80%; margin-left: 10%;height: 500px;"></div>

    <script type="text/javascript">
      function exportToCsv() {
          var valuesArray = ${lineGraphValuesJson};
          var performanceValuesArray = ${performanceValuesJson};
          var labelArray = ${labelsListJson};
          var myCsv;

          if (valuesArray.length > 0) {
              myCsv = valuesArray.join();
          } else {
              myCsv = performanceValuesArray.join();
              myCsv += '\n' + labelArray.join();
          }
          window.open('data:text/csv;charset=utf-8,' + escape(myCsv));
      }
      var button = document.getElementById("b");
      button.addEventListener("click", exportToCsv);

      function load() {
          var fromDate = $("#from").datepicker("getDate").getTime();
          var toDate = $("#to").datepicker("getDate").getTime();
          var startTime = fromDate * NANO_PER_MILLI;
          var endTime = (toDate - 1) * NANO_PER_MILLI + ONE_DAY;  // end of day
          var ctx = "${pageContext.request.contextPath}";
          var link = ctx + "/show_graph?profilingPoint=${profilingPointName}" +
              "&tableName=" + ${tableName} +
              "&buildIdStartTime=" + startTime +
              "&buildIdEndTime=" + endTime;
          window.open(link,"_self");
      }
      button = document.getElementById("load");
      button.addEventListener("click", load);

    </script>

</body>
</html>
