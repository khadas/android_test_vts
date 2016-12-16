<%-- //[START all]--%>
<%@ page contentType='text/html;charset=UTF-8' language='java' %>
<%@ taglib prefix='fn' uri='http://java.sun.com/jsp/jstl/functions' %>
<%@ taglib prefix='c' uri='http://java.sun.com/jsp/jstl/core'%>


<html>
  <link rel='icon' href='https://www.gstatic.com/images/branding/googleg/1x/googleg_standard_color_32dp.png' sizes='32x32'>
  <link rel='stylesheet' href='https://fonts.googleapis.com/icon?family=Material+Icons'>
  <link rel='stylesheet' href='https://fonts.googleapis.com/css?family=Roboto:100,300,400,500,700'>
  <link rel='stylesheet' href='https://www.gstatic.com/external_hosted/materialize/all_styles-bundle.css'>
  <link rel='stylesheet' href='/css/navbar.css'>
  <link rel='stylesheet' href='/css/show_preferences.css'>
  <script src='https://ajax.googleapis.com/ajax/libs/jquery/3.1.0/jquery.min.js'></script>
  <script src='https://www.gstatic.com/external_hosted/materialize/materialize.min.js'></script>
  <script type='text/javascript' src='https://ajax.googleapis.com/ajax/libs/jqueryui/1.12.0/jquery-ui.min.js'></script>
  <head>
    <title>VTS Dashboard</title>

    <nav id='navbar'>
      <div class='nav-wrapper'>
        <span>
          <a href='/' class='breadcrumb'>VTS Dashboard Home</a>
          <a href='#!' class='breadcrumb'>Preferences</a>
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
      </div>
    </nav>
    <script>
        var subscribedSet = new Set(${subscribedTestsJson});
        var displayedSet = new Set(${subscribedTestsJson});
        var allTests = ${allTestsJson};
        var testSet = new Set(allTests);
        var addedSet = new Set();
        var removedSet = new Set();

        var addFunction = function() {
            test = $('#input-box').val();
            if (testSet.has(test)) {
                var icon = $('<i></i>').addClass('material-icons')
                                       .html('clear');
                var clear = $('<a></a>').addClass('btn-flat clear-button')
                                        .append(icon)
                                        .click(clearFunction);
                var span = $('<span></span>').addClass('entry valign')
                                             .html(test);
                var div = $('<div></div>').addClass('col s12 card option valign-wrapper')
                                          .append(span).append(clear)
                                          .prependTo('#options')
                                          .hide()
                                          .slideDown(150);
                if (!subscribedSet.has(test)) {
                    addedSet.add(test);
                } else {
                    removedSet.delete(test);
                }
                $('#input-box').val('').focusout();
                if (!addedSet.size && !removedSet.size) {
                    $('#save-button-wrapper').slideUp(50);
                } else {
                    $('#save-button-wrapper').slideDown(50);
                }
            }
        }

        var clearFunction = function() {
            var div = $(this).parent();
            div.slideUp(150, function() {
                div.remove();
            });
            var test = div.find('span').text();
            displayedSet.delete(test);
            if (subscribedSet.has(test)) {
                removedSet.add(test);
            } else {
                addedSet.delete(test);
            }
            if (!addedSet.size && !removedSet.size) {
                $('#save-button-wrapper').slideUp(50);
            } else {
                $('#save-button-wrapper').slideDown(50);
            }
        }

        var submitForm = function() {
            var added = Array.from(addedSet).join(',');
            var removed = Array.from(removedSet).join(',');
            $('#prefs-form>input[name="addedTests"]').val(added);
            $('#prefs-form>input[name="removedTests"]').val(removed);
            $('#prefs-form').submit();
        }

        $.widget('custom.sizedAutocomplete', $.ui.autocomplete, {
            _resizeMenu: function() {
                this.menu.element.outerWidth($('#input-box').width());
            }
        });

        $(function() {
            $('#input-box').sizedAutocomplete({
                source: allTests,
                classes: {
                    'ui-autocomplete': 'card'
                }
            });

            $('#input-box').keyup(function(event) {
                if (event.keyCode == 13) {  // return button
                    $('#add-button').click();
                }
            });

            $('.clear-button').click(clearFunction);
            $('#add-button').click(addFunction);
            $('#save-button').click(submitForm);
            $('#save-button-wrapper').hide();
        });
    </script>
  </head>

  <body>
    <div class='container'>
      <div class='row'>
        <h3 class='col s12 header'>Favorites</h3>
        <p class='col s12 caption'>Add or remove tests from favorites to customize
            the dashboard. Tests in your favorites will send you email notifications
            to let you know when test cases change status.
        </p>
      </div>
      <div class='row'>
        <div class='input-field col s8'>
          <input type='text' id='input-box'></input>
          <label for='input-box'>Search for tests to add to favorites</label>
        </div>
        <div id='add-button-wrapper' class='col s1 valign-wrapper'>
          <a id='add-button' class='btn-floating btn waves-effect waves-light red valign'><i class='material-icons'>add</i></a>
        </div>
      </div>
      <div class='row' id='options'>
        <c:forEach items='${subscribedTests}' var='test'>
          <div class='col s12 card option valign-wrapper'>
            <span class='entry valign'>${test}</span>
            <a class='btn-flat clear-button'>
                <i class='material-icons'>clear</i>
            </a>
          </div>
        </c:forEach>
      </div>
    </div>
    <form id='prefs-form' style='visibility:hidden' action='/show_preferences' method='post'>
        <input name='addedTests' type='text'>
        <input name='removedTests' type='text'>
    </form>
    <div id='save-button-wrapper' class='fixed-action-btn'>
      <a id='save-button' class='btn-floating btn-large red waves-effect'>
        <i class='large material-icons'>done</i>
      </a>
    </div>
    <footer class='page-footer'>
      <div class='footer-copyright'>
        <div class='container'>
          © 2016 - The Android Open Source Project
        </div>
      </div>
    </footer>
  </body>
</html>
<%-- //[END all]--%>
