<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>

<html>
  <head>
    <link rel="icon" href="https://www.gstatic.com/images/branding/googleg/1x/googleg_standard_color_32dp.png" sizes="32x32">
    <link rel="stylesheet" href="https://fonts.googleapis.com/icon?family=Material+Icons">
    <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Roboto:100,300,400,500,700">
    <link rel="stylesheet" href="https://www.gstatic.com/external_hosted/materialize/all_styles-bundle.css">
    <link type="text/css" href="/css/navbar.css" rel="stylesheet">
    <link type="text/css" href="/css/show_graph.css" rel="stylesheet">
    <link rel="stylesheet" href="https://ajax.googleapis.com/ajax/libs/jqueryui/1.12.0/themes/smoothness/jquery-ui.css">
    <script type="text/javascript" src="https://ajax.googleapis.com/ajax/libs/jquery/3.1.0/jquery.min.js"></script>
    <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
    <script type="text/javascript" src="https://ajax.googleapis.com/ajax/libs/jqueryui/1.12.0/jquery-ui.min.js"></script>
    <script src="https://www.gstatic.com/external_hosted/materialize/materialize.min.js"></script>
    <title>Graph</title>
    <script type="text/javascript">
      google.charts.load("current", {packages:["corechart", "table", "line"]});
      google.charts.setOnLoadCallback(drawProfilingChart);
      google.charts.setOnLoadCallback(drawPerformanceChart);
      google.charts.setOnLoadCallback(drawPercentileTable);

      ONE_DAY = 86400000000;
      MICRO_PER_MILLI = 1000;

      $(function() {
          var startTime = ${startTime};
          var endTime = ${endTime};
          if (!startTime || !endTime) {
              var now = (new Date()).getTime()*MICRO_PER_MILLI;
              startTime = now - ONE_DAY;
              endTime = now;
          }
          var fromDate = new Date(startTime/MICRO_PER_MILLI);
          var toDate = new Date(endTime/MICRO_PER_MILLI);
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
          var showPerformanceGraph = ${showPerformanceGraph};
          if (!showPerformanceGraph) {
              return;
          }
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
          data.addColumn('string', ${performanceLabelX});
          profilingBuildIds.forEach(function(build) {
              data.addColumn('number', build);
          });
          data.addRows(lineGraphValues);

          var options = {
            chart: {
                title: 'Performance',
                subtitle: ${performanceLabelY}
            },
            legend: { position: 'none' }
          };
          var chart = new google.charts.Line(document.getElementById('performance_chart_div'));
          chart.draw(data, options);
      }

      // table for profiling data
      function drawPercentileTable() {
          var showProfilingGraph = ${showProfilingGraph};
          if (!showProfilingGraph) {
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
    <nav id="navbar">
      <div class="nav-wrapper">
        <div class="col s12">
          <a href="${pageContext.request.contextPath}/" class="breadcrumb">VTS Dashboard Home</a>
          <a href="${pageContext.request.contextPath}/show_table?testName=${testName}&startTime=${startTime}&endTime=${endTime}" class="breadcrumb">${testName}</a>
          <a href="#!" class="breadcrumb">Profiling</a>
        </div>
      </div>
    </nav>
  </head>

  <body>
    <div id="download" class="fixed-action-btn">
      <a id="b" class="btn-floating btn-large red waves-effect waves-light">
        <i class="large material-icons">file_download</i>
      </a>
    </div>
    <div class="container">
      <div class="row card">
        <div id="header-container" class="valign-wrapper col s12">
          <div class="col s3 valign">
            <h5>Profiling Point:</h5>
          </div>
          <div class="col s9 right-align valign">
            <h5 class="profiling-name truncate">${profilingPointName}</h5>
          </div>
        </div>
        <div id="date-container" class="col s12">
          <input type="text" id="from" name="from" class="col s2 offset-s7">
          <input type="text" id="to" name="to" class="col s2">
          <a id="load" class="btn-floating btn-medium red right waves-effect waves-light">
            <i class="medium material-icons">cached</i>
          </a>
        </div>
      </div>
      <c:if test="${showProfilingGraph}">
        <div id="profiling-container" class="row card">
          <div class="col s10 offset-s1 center-align">
            <!-- Profiling chart for profiling values. -->
            <div id="profiling_chart_div" style="width: 80%; height: 500px;"></div>

            <!-- Percentile table -->
            <div id="percentile_table_div" style="margin-left:10%; margin-top:-40px; margin-bottom:125px"></div>
          </div>
        </div>
      </c:if>
      <c:if test="${showPerformanceGraph}">
        <div id="performance-container" class="row card">
          <!-- Performance chart for label vs values. -->
          <div id="performance_chart_div" style="width:80%; margin-left: 10%;height: 500px;"></div>
        </div>
      </c:if>
      <c:if test="${not empty error}">
        <div id="error-container" class="row card">
          <div class="col s10 offset-s1 center-align">
            <!-- Error in case of profiling data is missing -->
            <h5>${error}</h5>
          </div>
        </div>
      </c:if>
    </div>

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
      $("#b").click(exportToCsv);

      function load() {
          var fromDate = $("#from").datepicker("getDate").getTime();
          var toDate = $("#to").datepicker("getDate").getTime();
          var startTime = fromDate * MICRO_PER_MILLI;
          var endTime = (toDate - 1) * MICRO_PER_MILLI + ONE_DAY;  // end of day
          var ctx = "${pageContext.request.contextPath}";
          var link = ctx + "/show_graph?profilingPoint=${profilingPointName}" +
              "&testName=${testName}" +
              "&startTime=" + startTime +
              "&endTime=" + endTime;
          window.open(link,"_self");
      }
      $("#load").click(load);
    </script>
  </body>
</html>
