cr.define("account", function() {
  "use strict"

  const url = "http://180.167.10.100/browser/statistic/show_in_browser.php";
  const link = "http://browser.nfschina.com/browser/public/?rid=";
  var global_id;

  function formatSeconds(value) {
    let theTime = parseInt(value);// 秒
    let theTime1 = 0;// 分
    let theTime2 = 0;// 小时

    if(theTime > 60) {
      theTime1 = parseInt(theTime/60);
      theTime = parseInt(theTime%60);
      if(theTime1 > 60) {
        theTime2 = parseInt(theTime1/60);
        theTime1 = parseInt(theTime1%60);
      }
    }
    var result = ""+parseInt(theTime)+"秒";
    if(theTime1 > 0) {
        result = ""+parseInt(theTime1)+"分"+result;
    }
    if(theTime2 > 0) {
        result = ""+parseInt(theTime2)+"小时"+result;
    }
    return result;
  }

  function addEventListener() {
    $("submit").addEventListener("click", login);
    $("logout-btn").addEventListener("click", logout);
    $("bookmark").addEventListener("click", checkboxChanged);
    $("skin").addEventListener("click", checkboxChanged);
    $("password_checkbox").addEventListener("click", checkboxChanged);
    $("sync-btn").addEventListener("click", syncNow);

    $("copy-link").addEventListener("click", copyLink);

    document.addEventListener("contextmenu", function(e) {
      e.preventDefault();
    });
  }

  function syncNow() {
    chrome.send("syncNow");
    $("btn-sync").removeAttribute("hidden");
    setTimeout("document.getElementById('btn-sync').setAttribute('hidden', true)",2000)
  }


  function copyLink() {
    let oInput = document.createElement('input');
    oInput.value = link + global_id;

    document.body.appendChild(oInput);

    oInput.select(); // 选择对象
    document.execCommand("Copy"); // 执行浏览器复制命令
    oInput.setAttribute("hidden", true);
    $("btn-copy").removeAttribute("hidden");
    setTimeout("document.getElementById('btn-copy').setAttribute('hidden', true)",2000)
  }


  function getAccountId(id) {
    global_id = id;
  }

  function isLoggedIn(result) {
    if (result) {
      $("manage-page").removeAttribute("hidden");
      chrome.send("getEmail", []);
      chrome.send("getOptions", []);
    } else {
      $("login-page").removeAttribute("hidden");
    }
  }

  function enterLogin() {
    document.getElementById('email').addEventListener('keydown', function(e) {
      if (e.keyCode == 13) {
        login();
      }
    });

    document.getElementById('password').addEventListener('keydown', function(e) {
      if (e.keyCode == 13) {
        login();
      }
    });
  }

  function checkboxChanged() {
    var result = document.getElementById("bookmark").checked;
    chrome.send("changeSyncBookmark", [result]);
    result = document.getElementById("skin").checked;
    chrome.send("changeSyncSkin", [result]);
    result = document.getElementById("password_checkbox").checked;
    chrome.send("changeSyncPassword", [result]);
  }


  function onLoginResult(error_code, error_message) {
    if (0 == error_code) {
      $("login-page").setAttribute("hidden", "");
      $("manage-page").removeAttribute("hidden");
      chrome.send("getEmail", []);
    } else if (1 == error_code || 10 == error_code) {
      $("result").innerText = error_message;
    } else {
      $("result").innerText = "网络错误，请稍后重试！";
    }
  }

  function showEmail(email) {
    $("email-name").querySelector("span").innerText = email;
  }

  function showOptions(is_sync_bookmark, is_sync_skin, is_sync_password) {
    if(is_sync_bookmark == false)
      $("bookmark").removeAttribute("checked");

    if(is_sync_skin == false)
      $("skin").removeAttribute("checked");

    if(is_sync_password == false)
      $("password_checkbox").removeAttribute("checked");
  }

  function login() {
    let email = $("email").value;
    let password = $("password").value;
    if (email.length === 0 || password.length === 0) {
      $("result").innerText = "请输入用户名和密码！";
      return;
    }

    console.log("login");
    chrome.send("login", [email, password]);
  }

  function logout() {
    chrome.send("logout", []);
  }

  function logoutSuccess() {
      $("manage-page").setAttribute("hidden", "");
      $("login-page").removeAttribute("hidden");
      $("result").innerText = "";
  }

  function onLoad() {
    chrome.send("getAccountId");
    chrome.send("isLoggedIn", []);

    enterLogin();
    addEventListener();
  }

  return {
    getAccountId: getAccountId,
    isLoggedIn: isLoggedIn,
    loginResult: onLoginResult,
    logoutSuccess: logoutSuccess,
    onLoad: onLoad,
    showEmail: showEmail,
    showOptions: showOptions,
  };
});

document.addEventListener('DOMContentLoaded', account.onLoad);
