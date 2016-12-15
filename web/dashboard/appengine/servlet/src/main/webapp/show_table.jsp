<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>


<html>
<head>
      <title>VTS Table</title>
</head>
<body>
      <h1>Table : ${tableName}</h1>
      <!-- Error in case of profiling data is missing -->
      <h3>${error}</h3>

      <h3>List of Test Cases</h3>
      <c:forEach items="${profilingPointNameArray}" var="profilingPoint">
            <a href="${pageContext.request.contextPath}/show_graph?profilingPoint=${profilingPoint}">${profilingPoint}</a>
          <br>
      </c:forEach>
</body>
</html>
