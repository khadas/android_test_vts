<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>


<html>
<head>
    <title>Graph</title>
    <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
    <script type="text/javascript">
      google.charts.load("current", {packages:["corechart"]});
      google.charts.setOnLoadCallback(drawChart);

      function drawChart() {

        // retrieve values from Servlet through JSON passed through sessionScope
        var valuesArray = ${valuesJson};
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

          chartArea: { width: 401 },
          hAxis: {
            ticks: histogramTicks
          },
          bar: { gap: 0 },

          histogram: {
            bucketSize: 0.02,
            maxNumBuckets: 200,
            minValue: min,
            maxValue: max
          }
        };
        var chart = new google.visualization.Histogram(document.getElementById('chart_div'));
        chart.draw(data, options);
      }

      function saveTextAsFile() {
          var textToSave = ${valuesJson};
          var filename = ${profilingPointName};
          var blob = new Blob([textToSave], {type: "text/plain;charset=utf-8"});
          saveAs(blob, filename+".txt");
      }
    </script>
</head>

<body>
    <div style="margin-left:200px">
      <h2>Profiling Point Name : ${profilingPointName}</h2>
      <button id="b">Click to Download Raw Data </button>
      <!-- Error in case of profiling data is missing -->
      <h3>${error}</h3>
    </div>

    <div id="chart_div" style="width: 900px; height: 500px;"></div>

    <div style="margin-left:200px">
      <table border="1" cellpadding="5" cellspacing="5">
          <tr>
            <th>10</th>
            <th>25</th>
            <th>50</th>
            <th>75</th>
            <th>80</th>
            <th>90</th>
            <th>95</th>
            <th>99</th>
          </tr>
          <tr>
          <c:forEach items="${percentileResultArray}" var="value">
              <td>${value}</td>
          </c:forEach>
          </tr>
      </table>
    </div>
    <script type="text/javascript">
      function exportToCsv() {
          var valuesArray = ${valuesJson};
          var myCsv = valuesArray.join();
          window.open('data:text/csv;charset=utf-8,' + escape(myCsv));
      }
      var button = document.getElementById('b');
      button.addEventListener('click', exportToCsv);

    </script>

</body>
</html>
