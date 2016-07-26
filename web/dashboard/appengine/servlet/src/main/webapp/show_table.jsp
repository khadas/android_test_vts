<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>


<html>
<head>
      <title>VTS Dashboard</title>
</head>
<body>
      <h1>Table : ${table_name}</h1>
      <h2>Raw Data (Unit : milliseconds) </h2>
      <!-- Error in case of profiling data is missing -->
      <h3>${error}</h3>

      <c:forEach items="${map}" var="entry">
          Test Case Name = ${entry.key} <br>
          Time Taken = ${entry.value} <br> <br>
      </c:forEach>

      <c:forEach items="${resultMap}" var="entry">
          <h3>Test Case Name: ${entry.key}</h3> <br>

            <c:forEach items="${entry.value}" var="item">
                ${item}<br> <br>
            </c:forEach>
      </c:forEach>

</body>
</html>
