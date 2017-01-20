<%--
  ~ Copyright (c) 2016 Google Inc. All Rights Reserved.
  ~
  ~ Licensed under the Apache License, Version 2.0 (the "License"); you
  ~ may not use this file except in compliance with the License. You may
  ~ obtain a copy of the License at
  ~
  ~     http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing, software
  ~ distributed under the License is distributed on an "AS IS" BASIS,
  ~ WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
  ~ implied. See the License for the specific language governing
  ~ permissions and limitations under the License.
  --%>
<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>

<html>
  <head>
    <link rel='icon' href='https://www.gstatic.com/images/branding/googleg/1x/googleg_standard_color_32dp.png' sizes='32x32'>
    <link rel='stylesheet' href='https://fonts.googleapis.com/icon?family=Material+Icons'>
    <link rel='stylesheet' href='https://fonts.googleapis.com/css?family=Roboto:100,300,400,500,700'>
    <link rel='stylesheet' href='https://www.gstatic.com/external_hosted/materialize/all_styles-bundle.css'>
    <link type='text/css' href='/css/navbar.css' rel='stylesheet'>
    <link type='text/css' href='/css/datepicker.css' rel='stylesheet'>
    <link type='text/css' href='/css/show_graph.css' rel='stylesheet'>
    <link rel='stylesheet' href='https://ajax.googleapis.com/ajax/libs/jqueryui/1.12.0/jquery-ui.css'>
    <script src='/js/analytics.js' type='text/javascript'></script>
    <script type='text/javascript' src='https://ajax.googleapis.com/ajax/libs/jquery/3.1.0/jquery.min.js'></script>
    <script src='https://www.gstatic.com/external_hosted/materialize/materialize.min.js'></script>
    <script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>
    <script type='text/javascript' src='https://ajax.googleapis.com/ajax/libs/jqueryui/1.12.1/jquery-ui.min.js'></script>
    <script src='/js/analytics.js' type='text/javascript'></script>
    <title>Graph</title>
    <script type='text/javascript'>
      if (${analytics_id}) analytics_init(${analytics_id});
      google.charts.load('current', {packages:['corechart', 'table', 'line']});
      google.charts.setOnLoadCallback(drawAllLineGraphs);
      google.charts.setOnLoadCallback(function() {
          $('.gradient').removeClass('gradient');
      });

      MICRO_PER_MILLI = 1000;

      $(function() {
          $('select').material_select();
          var date = $('#date').datepicker({
                  showAnim: 'slideDown',
                  maxDate: new Date()
                });
          date.datepicker('setDate', new Date(${startTime} / MICRO_PER_MILLI));
          $('#load').click(load);
      });

      function drawAllLineGraphs() {
          var graphs = ${graphs};
          graphs.forEach(function(graph) {
              if (graph.type == 'LINE_GRAPH') drawLineGraph(graph);
          });
      }

      function drawLineGraph(lineGraph) {
          var title = 'Performance';
          if (lineGraph.name) title += ' (' + lineGraph.name + ')';
          if (lineGraph.ticks.length < 1) {
              return;
          }
          lineGraph.ticks.forEach(function (label, i) {
              lineGraph.values[i].unshift(label);
          });
          var data = new google.visualization.DataTable();
          data.addColumn('string', lineGraph.x_value);
          lineGraph.ids.forEach(function(id) {
              data.addColumn('number', id);
          });
          data.addRows(lineGraph.values);
          var options = {
            chart: {
                title: title,
                subtitle: lineGraph.y_label
            },
            legend: { position: 'none' }
          };
          var container = $('<div class="row card center-align col s12 graph"></div>');
          container.appendTo('#profiling-container');
          var chart = new google.charts.Line(container[0]);
          chart.draw(data, options);
      }

      function load() {
          var startTime = $('#date').datepicker('getDate').getTime();
          var ctx = '${pageContext.request.contextPath}';
          var link = ctx + '/show_graph?profilingPoint=${profilingPointName}' +
              '&testName=${testName}' +
              '&startTime=' + (startTime * MICRO_PER_MILLI);
          if ($('#device-select').prop('selectedIndex') > 1) {
              link += '&device=' + $('#device-select').val();
          }
          window.open(link,'_self');
      }

    </script>
    <nav id='navbar'>
      <div class='nav-wrapper'>
        <span>
          <a href='${pageContext.request.contextPath}/' class='breadcrumb'>VTS Dashboard Home</a>
          <a href='${pageContext.request.contextPath}/show_table?testName=${testName}&startTime=${startTime}' class='breadcrumb'>${testName}</a>
          <a href='#!' class='breadcrumb'>Profiling</a>
        </span>
        <ul class='right'><li>
          <a id='dropdown-button' class='dropdown-button btn red lighten-3' href='#' data-activates='dropdown'>
            ${email}
          </a>
        </li></ul>
        <ul id='dropdown' class='dropdown-content'>
          <li><a href='${logoutURL}'>Log out</a></li>
        </ul>
      </div>
    </nav>
  </head>

  <body>
    <div id='download' class='fixed-action-btn'>
      <a id='b' class='btn-floating btn-large red waves-effect waves-light'>
        <i class='large material-icons'>file_download</i>
      </a>
    </div>
    <div class='container'>
      <div class='row card'>
        <div id='header-container' class='valign-wrapper col s12'>
          <div class='col s3 valign'>
            <h5>Profiling Point:</h5>
          </div>
          <div class='col s9 right-align valign'>
            <h5 class='profiling-name truncate'>${profilingPointName}</h5>
          </div>
        </div>
        <div id='date-container' class='col s12'>
          <div id='device-select-wrapper' class='input-field col s6 m3 offset-m6'>
            <select id='device-select'>
              <option value='' disabled>Select device</option>
              <option value='0' ${empty selectedDevice ? 'selected' : ''}>All Devices</option>
              <c:forEach items='${devices}' var='device' varStatus='loop'>
                <option value=${device} ${selectedDevice eq device ? 'selected' : ''}>${device}</option>
              </c:forEach>
            </select>
          </div>
          <input type='text' id='date' name='date' class='col s5 m2'>
          <a id='load' class='btn-floating btn-medium red right waves-effect waves-light'>
            <i class='medium material-icons'>cached</i>
          </a>
        </div>
      </div>
      <div id='profiling-container'>
      </div>
      <c:if test='${not empty error}'>
        <div id='error-container' class='row card'>
          <div class='col s10 offset-s1 center-align'>
            <!-- Error in case of profiling data is missing -->
            <h5>${error}</h5>
          </div>
        </div>
      </c:if>
    </div>
  </body>
</html>
