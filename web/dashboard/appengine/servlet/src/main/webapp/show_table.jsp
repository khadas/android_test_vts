<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>

<html>
  <head>
      <title>VTS Table</title>
      <style type="text/css">
          table.table_grid {
            color: #333;
            font-family: Helvetica, Arial, sans-serif;
            width: 640px;
            /* Table reset stuff */
            border: 3px dotted;
          }

          table.table_grid td, th {  border: 0 none; height: 30px;}

          table.table_grid th {
            /* Gradient Background */
            background: linear-gradient(#333 0%,#444 100%);
            color: #FFF; font-weight: bold;
            height: 40px;
          }

          table.table_grid td { background: #FAFAFA; text-align: center; }

          /* Zebra Stripe Rows */

          table.table_grid tr:nth-child(even) table.table_grid td { background: #EEE; }
          table.table_grid tr:nth-child(odd) table.table_grid td { background: #FDFDFD; }

          /* First-child blank cells! */
          table.table_grid tr td:first-child, table.table_grid tr th:first-child {
            background: none;
            font-style: italic;
            font-weight: bold;
            font-size: 14px;
            text-align: right;
            padding-right: 10px;
            width: 80px;
          }

          /* Add border-radius to specific cells! */
          table.table_grid tr:first-child table.table_grid th:nth-child(2) {
            border-radius: 5px 0 0 0;
          }

          table.table_grid tr:first-child table.table_grid th:last-child {
            border-radius: 0 5px 0 0;
          }

          /*Table to list profiling point names.*/
          table.table_list {
            color: #333;
            font-family: Helvetica, Arial, sans-serif;
            width: 640px;
            border-collapse:
            collapse; border-spacing: 0;
            }

            td, th {
            border: 1px solid transparent; /* No more visible border */
            height: 30px;
            transition: all 0.3s;  /* Simple transition for hover effect */
            }

            th {
            background: #551A8B;  /* Darken header a bit */
            font-weight: bold;
            color: white;
            }

            td {
            background: #912CEE;
            text-align: center;
            }

            /* Cells in even rows (2,4,6...) are one color */
            tr:nth-child(even) td { background: #EED2EE; }
            /* Cells in odd rows (1,3,5...) are another (excludes header cells)  */
            tr:nth-child(odd) td { background: #EAADEA; }
            tr td:hover { background: #DA70D6; color: #FFF; } /* Hover cell effect! */
      </style>
  </head>

  <body>
    <div style="margin-left:200px">
        <h2>Table : ${tableName}</h2>
        <!-- Error in case of profiling data is missing -->
        <h3>${error}</h3>
        <table class="table_list">
            <tr>
              <th>Profiling Point Names</th>
            </tr>

            <c:forEach items="${profilingPointNameArray}" var="profilingPoint">
                <tr>
                    <td><a href="${pageContext.request.contextPath}/show_graph?profilingPoint=${profilingPoint}">${profilingPoint}</a></td>
                </tr>
            </c:forEach>
      </table>
    </div>

    <div style="margin-left:200px">
    <br/><br/>
      <h3>Test cases and build</h3>
      <table class="table_grid">
          <tr>
              <th>Test Case / Build ID</th>
              <!-- y dimemsion for matrix: Build IDs come here -->
              <c:forEach items="${buildNameArray}" var="buildName">
                  <th>${buildName}</th>
              </c:forEach>
          </tr>
           <!-- y dimension for matrix -->
          <c:forEach items="${testCaseReportMessagesArray}" var="testCaseReportMessage">
              <tr>
                  <td>${testCaseReportMessage}</td>
              </tr>
          </c:forEach>
      </table>
    </div>

  </body>
</html>
