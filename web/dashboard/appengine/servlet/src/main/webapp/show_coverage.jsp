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
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions" %>
<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core"%>


<html>
  <head>
    <title>Coverage Information</title>
    <link rel="icon" href="https://www.gstatic.com/images/branding/googleg/1x/googleg_standard_color_32dp.png" sizes="32x32">
    <link rel="stylesheet" href="https://fonts.googleapis.com/icon?family=Material+Icons">
    <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Roboto:100,300,400,500,700">
    <link rel="stylesheet" href="https://www.gstatic.com/external_hosted/materialize/all_styles-bundle.css">
    <link rel="stylesheet" href="/css/navbar.css">
    <link rel="stylesheet" href="/css/show_coverage.css">
    <script src='/js/analytics.js' type='text/javascript'></script>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.1.0/jquery.min.js"></script>
    <script src="https://www.gstatic.com/external_hosted/materialize/materialize.min.js"></script>
    <script src="https://apis.google.com/js/api.js" type="text/javascript"></script>
    <script type="text/javascript">
        if (${analytics_id}) analytics_init(${analytics_id});
        var coverageVectors = ${coverageVectors};
        $(document).ready(function() {
            // Initialize AJAX for CORS
            $.ajaxSetup({
                xhrFields : {
                    withCredentials: true
                }
            });

            // Initialize auth2 client and scope for requests to Gerrit
            gapi.load('auth2', function() {
                var auth2 = gapi.auth2.init({
                    client_id: ${clientId},
                    scope: ${gerritScope}
                });
                auth2.then(displayEntries);
            });
        });

        /* Loads source code for a particular entry and displays it with
           coverage information as the accordion entry expands.
        */
        var onClick = function() {
            // Remove source code from the accordion entry that was open before
            var prev = $(this).parent().find('li.active');
            if (prev.length > 0) {
                prev.find('.table-container').empty();
            }
            var url = $(this).attr('url');
            var i = $(this).attr('index');
            var table = $(this).find('.table-container');
            table.html('<div class="center-align">Loading...</div>');
            if ($(this).hasClass('active')) {
                // Remove the code from display
                table.empty();
            } else {
                /* Fetch and display the code.
                   Note: a coverageVector may be shorter than sourceContents due
                   to non-executable (i.e. comments or language-specific syntax)
                   lines in the code. Trailing source lines that have no
                   coverage information are assumed to be non-executable.
                */
                $.ajax({
                    url: url,
                    dataType: 'text'
                }).promise().done(function(src) {
                    src = atob(src);
                    if (!src) return;
                    srcLines = src.split('\n');
                    covered = 0;
                    total = 0;
                    var rows = srcLines.reduce(function(acc, line, j) {
                        var count = coverageVectors[i][j];
                        if (typeof count == 'undefined' || count < 0) {
                            acc += '<tr>';
                            count = "--";
                        } else if (count == 0) {
                            acc += '<tr class="uncovered">';
                            total += 1;
                        } else {
                            acc += '<tr class="covered">';
                            covered += 1;
                            total += 1;
                        }
                        acc += '<td class="count">' + String(count) + '</td>';
                        acc += '<td class="line_no">' + String(j+1) + '</td>';
                        acc += '<td class="code">' + String(line) + '</td></tr>';
                        return acc;
                    }, String());
                    table.html('<table class="table">' + rows + '</table>');
                }).fail(function() {
                    table.html('<div class="center-align">Not found.</div>');
                });
            }
        }

        /* Appends a row to the display with test name and aggregated coverage
           information. On expansion, source code is loaded with coverage
           highlighted by calling 'onClick'.
        */
        var displayEntries = function() {
            var sourceFilenames = ${sourceFiles};
            var sectionMap = ${sectionMap};
            var gerritURI = ${gerritURI};
            var projects = ${projects};
            var commits = ${commits};
            var indicators = ${indicators};
            Object.keys(sectionMap).forEach(function(section) {
                var indices = sectionMap[section];
                var html = String();
                indices.forEach(function(i) {
                    var url = gerritURI + '/projects/' +
                              encodeURIComponent(projects[i]) + '/commits/' +
                              encodeURIComponent(commits[i]) + '/files/' +
                              encodeURIComponent(sourceFilenames[i]) +
                              '/content';
                    html += '<li onclick="onClick" url="' + url + '" index="' +
                            i + '"><div class="collapsible-header">' +
                            '<i class="material-icons">library_books</i>' +
                            sourceFilenames[i] + indicators[i] + '</div>';
                    html += '<div class="collapsible-body row">' +
                            '<div class="html-container">' +
                            '<div class="table-container"></div>' +
                            '</div></div></li>';
                });
                if (html) {
                    html = '<h4 class="section-title"><b>Coverage:</b> ' +
                           section + '</h4><ul class="collapsible popout" ' +
                           'data-collapsible="accordion">' + html + '</ul>';
                    $('#coverage-container').append(html);
                }
            });
            $('.collapsible.popout').collapsible({
               accordion : true
            }).children().click(onClick);
        }
    </script>
    <nav id="navbar">
      <div class="nav-wrapper">
        <span>
          <a href="${pageContext.request.contextPath}/" class="breadcrumb">VTS Dashboard Home</a>
          <a href="${pageContext.request.contextPath}/show_table?testName=${testName}&startTime=${startTime}&endTime=${endTime}" class="breadcrumb">${testName}</a>
          <a href="#!" class="breadcrumb">Coverage</a>
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
    <div id="coverage-container" class="container">
    </div>
    <footer class="page-footer">
      <div class="footer-copyright">
        <div class="container">© 2016 - The Android Open Source Project
        </div>
      </div>
    </footer>
  </body>
</html>
