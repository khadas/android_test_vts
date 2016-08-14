<%-- //[START all]--%>
<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions" %>
<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core"%>


<html>
  <head>
    <title>VTS Dashboard</title>
    <style type="text/css">
      table {
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
    <h3>VTS Dashboard (Internal)</h3>
    <table id='dashboard_main_table'>
        <tr>
           <th>Test Name</th>
        </tr>
        <c:forEach items="${tableNames}" var="table">
                <tr>
                    <td><a href="${pageContext.request.contextPath}/show_table?tableName=result_${table}&buildIdPageNo=0">${table}</a></td>
                </tr>
        </c:forEach>
     </table>
    </div>
  </body>
</html>
<%-- //[END all]--%>
