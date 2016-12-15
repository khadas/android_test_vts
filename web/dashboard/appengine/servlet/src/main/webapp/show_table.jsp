<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>


<html>
<head>
      <title>VTS Table</title>
      <link rel="stylesheet" type="text/css" href="show_table.css">
</head>
<body>
  <div style="margin-left:200px">
      <h2>Table : ${tableName}</h2>
      <!-- Error in case of profiling data is missing -->
      <h3>${error}</h3>
      <table border="1" cellpadding="10" cellspacing="10">
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
    <h3>Test cases and build</h3>
    <table border="1" cellpadding="5" cellspacing="5">
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
