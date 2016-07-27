<%-- //[START all]--%>
<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions" %>
<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core"%>


<html>
  <head>
    <title>VTS Dashboard</title>
  </head>
  <body>
    <div style="margin-left:200px">
    <h3>Project : android-vts-internal</h3>
     <table border="1" cellpadding="10" cellspacing="10">
             <tr>
               <th>List of Tables</th>
            </tr>
        <c:forEach items="${tableNames}" var="table">
                <tr>
                    <td><a href="${pageContext.request.contextPath}/show_table?tableName=${table}">${table}</a></td>
                </tr>
        </c:forEach>
     </table>
    </div>
  </body>
</html>
<%-- //[END all]--%>
