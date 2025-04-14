// JavaScript Document
/*$.browser = {
	mozilla: /firefox/.test(navigator.userAgent.toLowerCase()),
	webkit: /webkit/.test(navigator.userAgent.toLowerCase()),
	opera: /opera/.test(navigator.userAgent.toLowerCase()),
	msie: /msie/.test(navigator.userAgent.toLowerCase()),

    ie8: navigator.userAgent.indexOf("MSIE 8.0") > 0 ? true : false,
    ie7: navigator.userAgent.indexOf("MSIE 7.0") > 0 ? true : false,
    ie6: navigator.userAgent.indexOf("MSIE 6.0") > 0 ? true : false

};
//$.browser.ielow = $.browser.ie8 || $.browser.ie7 || $.browser.ie6//ielow指ie6-8各个版本。*/

$.browser = (function(){
    var ua = navigator.userAgent.toLowerCase();
    /*var s;
    (s = ua.match(/msie ([\d.]+)/)) ? Sys.ie = s[1] :
    (s = ua.match(/firefox\/([\d.]+)/)) ? Sys.firefox = s[1] :
    (s = ua.match(/chrome\/([\d.]+)/)) ? Sys.chrome = s[1] :
    (s = ua.match(/opera.([\d.]+)/)) ? Sys.opera = s[1] :
    (s = ua.match(/version\/([\d.]+).*safari/)) ? Sys.safari = s[1] : 0;

    //以下进行测试
    if (Sys.ie) document.write('IE: ' + Sys.ie);
    if (Sys.firefox) document.write('Firefox: ' + Sys.firefox);
    if (Sys.chrome) document.write('Chrome: ' + Sys.chrome);
    if (Sys.opera) document.write('Opera: ' + Sys.opera);
    if (Sys.safari) document.write('Safari: ' + Sys.safari);*/




    var s,
        browser = {
            msie: (s = ua.match(/msie ([\d.]+)/)) ? s[1] : false,
            firefox: (s = ua.match(/firefox\/([\d.]+)/)) ? s[1] : false,
            chrome: (s = ua.match(/chrome\/([\d.]+)/)) ? s[1] : false,
            opera: (s = ua.match(/opera.([\d.]+)/)) ? s[1] : false,
            safari: (s = ua.match(/version\/([\d.]+).*safari/)) ? s[1] : false
        };

        browser.lelow = ((parseInt(browser.msie, 10)) < 8) ? true: false;

    return browser;
})();



//手机终端判断
$.isMobile = {
    Android: function() {
        return navigator.userAgent.match(/Android/i) ? true : false;
    },
    BlackBerry: function() {
        return navigator.userAgent.match(/BlackBerry/i) ? true : false;
    },
    iOS: function() {
        return navigator.userAgent.match(/iPhone|iPad|iPod/i) ? true : false;
    },
    Windows: function() {
        return navigator.userAgent.match(/IEMobile/i) ? true : false;
    },
    any: function() {
        return ($.isMobile.Android() || $.isMobile.BlackBerry() || $.isMobile.iOS() || $.isMobile.Windows());
    }
};

//文件动态加载函数
$.extend({
    includePath: '',
    include: function(file)
    {
        var files = typeof file == "string" ? [file] : file;
        for (var i = 0; i < files.length; i++)
        {
            var name = files[i].replace(/^\s|\s$/g, "");
            var att = name.split('.');
            var ext = att[att.length - 1].toLowerCase();
            var isCSS = ext == "css";
            var tag = isCSS ? "link" : "script";
            var attr = isCSS ? " type='text/css' rel='stylesheet' " : " language='javascript' type='text/javascript' ";
            var link = (isCSS ? "href" : "src") + "='" + $.includePath + name + "'";
            if ($(tag + "[" + link + "]").length == 0){
				 $("head").append("<" + tag + attr + link + "></" + tag + ">");
			};
        }
    }
});


/*
$.window = (function(){
    var me = new Object(),
        windowObj = $(window);

    me.width = windowObj.width();
    me.height = windowObj.height();
    me.getInfo = function(){
        me.width = windowObj.width();
        me.height = windowObj.height();
    };

    me = $.extend(me, windowObj);

    windowObj.on("resize load", me.getInfo);
    console.log(me)
    return me;
})();*/