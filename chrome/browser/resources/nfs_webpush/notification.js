function showNotification(icon, url, title, body) {
  // request permission on page load
  if (!Notification) {
    alert('Desktop notifications not available.'); 
    return;
  }

  if (Notification.permission !== "granted") {
    Notification.requestPermission();
  }  

  var notification = new Notification(title, {
     // icon is the url of the logo
      icon: icon,
      body: body,
  });  notification.onclick = function () {
    event.preventDefault();
    window.open(url);      
  };
}

function detectOS() {
    var sUserAgent = navigator.userAgent;
    var isWin = (navigator.platform == "Win32") || (navigator.platform == "Windows");
    var isMac = (navigator.platform == "Mac68K") || (navigator.platform == "MacPPC") || (navigator.platform == "Macintosh") || (navigator.platform == "MacIntel");
    if (isMac) return "Mac";
    var isUnix = (navigator.platform == "X11") && !isWin && !isMac;
    if (isUnix) return "Unix";
    var isLinux = (String(navigator.platform).indexOf("Linux") > -1);
    if (isLinux) return "Linux";
    if (isWin)  return "Windows";
    return "None";
}

function CheckNotification() {
  if (!localStorage.getItem('notification_version')) {
    localStorage.setItem('notification_version', 0);
  }

  var current_version = localStorage.getItem('notification_version');
  var notification_url = "http://172.31.50.100/webpush/index.php";
  var os = detectOS();

  $.ajax({
    url: notification_url,
    type: "GET",
    dataType:'jsonp',
    data:{
      'version': current_version,
      'os': os
   },
   jsonp:'callback',
   timeout:10000,
   async: true,
   success:function(result) {
        console.log("success");
        var isLatest = result['IsLatestVersion'];
        var error = result['error'];
        if (isLatest) {
          return;
        }

        // should show the notification
        var latest_version = result['latest_version'];
        if (latest_version != null) {
            localStorage.setItem('notification_version', latest_version);
        }

        showNotification(result['icon'], result['url'], result['title'], result['content']);
    },
    error: function (XMLHttpRequest, textStatus, errorThrown)  {
        console.log("error");
        console.log(XMLHttpRequest.status);
        // 状态
        console.log(XMLHttpRequest.readyState);
        // 错误信息   
        console.log(textStatus); 
    }
  });  

}

document.addEventListener('DOMContentLoaded', CheckNotification);