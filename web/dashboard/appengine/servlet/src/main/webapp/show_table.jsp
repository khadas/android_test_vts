<%-- //[START all]--%>
<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions" %>
<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core"%>


<html>
  <head>
  </head>

  <body>
  <h3>Table : ${table_name}</h3>
  <c:forEach items="${values}" var="value">
     ${value}
     <br>
  </c:forEach>
  </body>
</html>
<%-- //[END all]--%>
