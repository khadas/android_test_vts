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
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.1.0/jquery.min.js"></script>
    <script src="https://www.gstatic.com/external_hosted/materialize/materialize.min.js"></script>
    <script src="https://apis.google.com/js/api.js" type="text/javascript"></script>
    <script type="text/javascript">
        $(document).ready(function() {
            // Initialize AJAX for CORS
            $.ajaxSetup({
                xhrFields : {
                    withCredentials: true
                }
            });

            // Initialize auth2 client and scope for requests to Gerrit
            gapi.load('auth2', function() {
                auth2 = gapi.auth2.init({
                    client_id: ${clientId},
                    scope: ${gerritScope}
                });
                var sourceContents = [];
                // Read the source contents. Display the results when loaded.
                getSourceCode(sourceContents).done(
                    function() {
                        displaySource(sourceContents);
                        $('#coverage').collapsible({
                            accordion : true
                        });
                    }
                );
            });
        });

        /* Fetches the source code from Gerrit.
           Returns a promise that resolves when all of the requests complete.
        */
        var getSourceCode = function(sourceContents) {
            var gerritURI = ${gerritURI};
            var projects = ${projects};
            var commits = ${commits};
            var sourceNames = ${sourceFiles};
            var promises = sourceNames.map(function(src, i) {
                url = gerritURI + '/projects/' + encodeURIComponent(projects[i])
                        + '/commits/' + encodeURIComponent(commits[i])
                        + '/files/' + encodeURIComponent(src) + '/content';
                return $.ajax({
                    url: url,
                    dataType: 'text'
                }).promise();
            });
            return $.when.apply($, promises).always(function() {
                promises.forEach(function(d, i) {
                    d.done(function(result) {
                        sourceContents[i] = atob(result);
                    });
                });
            });
        }

        /* Appends source code to the DOM given the contents of the source code.
           Coverage vectors, source names, and test names passed from the
           servlet.
        */
        var displaySource = function(sourceContents) {
            /* Note: coverageVectors may be shorter than sourceContents due to
               non-executable (i.e. comments or language-specific syntax) lines
               in the code. Trailing source lines that have no coverage
               information are assumed to be non-executable.
            */
            var coverageVectors = ${coverageVectors};
            var sourceNames = ${sourceFiles};
            var testcaseNames = ${testcaseNames};
            sourceContents.forEach(function(src, i) {
                if (!src) return;
                srcLines = src.split('\n');
                rows = srcLines.reduce(function(acc, line, j) {
                    var count = coverageVectors[i][j];
                    if (typeof count == 'undefined' || count < 0) {
                        acc += '<tr>';
                        count = "--";
                    } else if (count == 0) {
                        acc += '<tr class="uncovered">';
                    } else {
                        acc += '<tr class="covered">';
                    }
                    acc += '<td class="count">' + String(count) + '</td>';
                    acc += '<td class="line_no">' + String(j+1) + '</td>';
                    acc += '<td class="code">' + String(line) + '</td></tr>';
                    return acc;
                }, String());
                html = '<li><div class="collapsible-header">';
                html += '<i class="material-icons">library_books</i>';
                html += testcaseNames[i];
                html += '<div class="right">' + sourceNames[i] + '</div></div>';
                html += '<div class="collapsible-body row">';
                html += '<div id="html_container" class="col s10 push-s1">';
                html += '<div><table class="table">' + rows + '</table></div>';
                html += '</div></div></li>';
                $('#coverage').append(html);
            });
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
    <div class="container">
      <ul class="collapsible popout" data-collapsible="accordion" id="coverage">
      </ul>
    </div>
    <footer class="page-footer">
      <div class="footer-copyright">
        <div class="container">© 2016 - The Android Open Source Project
        </div>
      </div>
    </footer>
  </body>
</html>
