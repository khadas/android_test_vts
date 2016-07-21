<%-- //[START all]--%>
<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions" %>
<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core"%>


<html>
  <head>
  </head>
  <body>
  <h3>List of Tables : android-vts-internal</h3>
      <c:forEach items="${tableNames}" var="table">
            <a href="${pageContext.request.contextPath}/show_table?tableName=${table}">${table}</a>
          <br>
      </c:forEach>
  </body>
</html>
<%-- //[END all]--%>
