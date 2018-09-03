(function(){
	//获取客户端操作系统类型
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

	var os = detectOS();
	var os="Windows";
	var linuxTitle = document.getElementById("linux_os");
	var windowsTitle = document.getElementById("windows_os");
	if(os=="Linux"){
		linuxTitle.style.display="block";
		windowsTitle.style.display="none";
	}else if(os == "Windows"){
		linuxTitle.style.display="none";
		windowsTitle.style.display="block";
	}else{
		console.log("other os");
	}
	var href = window.location.href;
	var ver = href.split("?")[1].split("&")[0];
	var core = href.split("?")[1].split("&")[1];
	document.querySelector(".aboutmain .version").innerHTML =loadTimeData.getString('version') +' '+ ver ;
	document.querySelector(".aboutmain .core").innerHTML =loadTimeData.getString('core') +' '+ core;

  document.getElementById("help_chromium").addEventListener("click", function() {
  	chrome.send("openChromiumUrl", []);
  }, false)
  document.getElementById("help_opensource").addEventListener("click", function() {
  	chrome.send("openSourceUrl", []);
  }, false)
  document.getElementById("help_help").addEventListener("click", function() {
  	chrome.send("openHelpUrl", []);
  }, false)
})();