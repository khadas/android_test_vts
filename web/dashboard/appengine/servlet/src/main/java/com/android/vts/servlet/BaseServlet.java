package com.android.vts.servlet;

import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.gson.Gson;
import java.io.IOException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.util.logging.Logger;

public abstract class BaseServlet extends HttpServlet {
    protected final Logger logger = Logger.getLogger(getClass().getName());

    // Environment variables
    protected static final String EMAIL_DOMAIN = System.getenv("EMAIL_DOMAIN");
    protected static final String SENDER_EMAIL = System.getenv("SENDER_EMAIL");
    protected static final String DEFAULT_EMAIL = System.getenv("DEFAULT_EMAIL");
    protected static final String GERRIT_URI = System.getenv("GERRIT_URI");
    protected static final String GERRIT_SCOPE = System.getenv("GERRIT_SCOPE");
    protected static final String CLIENT_ID = System.getenv("CLIENT_ID");
    protected static final String ANALYTICS_ID = System.getenv("ANALYTICS_ID");

    // Common constants
    protected static final long ONE_DAY = 86400000000L;  // units microseconds
    protected static final String TABLE_PREFIX = "result_";

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        // If the user is logged out, allow them to log back in and return to the page.
        // Set the logout URL to direct back to a login page that directs to the current request.
        UserService userService = UserServiceFactory.getUserService();
        User currentUser = userService.getCurrentUser();
        String loginURI = userService.createLoginURL(request.getRequestURI());
        String logoutURI = userService.createLogoutURL(loginURI);
        if (currentUser == null || currentUser.getEmail() == null) {
            response.sendRedirect(loginURI);
            return;
        }
        request.setAttribute("logoutURL", logoutURI);
        request.setAttribute("email", currentUser.getEmail());
        request.setAttribute("analytics_id", new Gson().toJson(ANALYTICS_ID));
        response.setContentType("text/html");
        doGetHandler(request, response);
    }

    /**
     * Implementation of the doGet method to be executed by servlet subclasses.
     * @param request The HttpServletRequest object.
     * @param response The HttpServletResponse object.
     * @throws IOException
     */
    public abstract void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException;
}
