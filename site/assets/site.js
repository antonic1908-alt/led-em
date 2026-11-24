/* Посібник: навігація, зміст сторінки, тема, копіювання коду, чек-листи. Без зовнішніх бібліотек. */
(function () {
  "use strict";

  var PAGES = [
    { file: "index.html",            num: "",   title: "Головна", group: "Старт" },
    { file: "01-learning-plan.html", num: "01", title: "План навчання", group: "Посібник" },
    { file: "02-ai-tools.html",      num: "02", title: "ШІ-інструменти і вайбкодинг", group: "Посібник" },
    { file: "03-software.html",      num: "03", title: "Програмне забезпечення", group: "Посібник" },
    { file: "04-simulators.html",    num: "04", title: "Симулятори та онлайн-інструменти", group: "Посібник" },
    { file: "05-hardware.html",      num: "05", title: "Компоненти і схема", group: "Посібник" },
    { file: "lessons.html",          num: "Л",  title: "Уроки 01–08", group: "Практика" },
    { file: "06-project.html",       num: "06", title: "Проєкт: розумна стрічка", group: "Практика" },
    { file: "07-stm32.html",         num: "07", title: "Перенесення на STM32", group: "Практика" },
    { file: "glossary.html",         num: "Aa", title: "Словник термінів", group: "Довідка" }
  ];

  function store(key, val) {
    try {
      if (val === undefined) return window.localStorage.getItem(key);
      window.localStorage.setItem(key, val);
    } catch (e) { return null; }
    return null;
  }

  var here = (location.pathname.split("/").pop() || "index.html").toLowerCase();
  var idx = 0;
  PAGES.forEach(function (p, i) { if (p.file === here) idx = i; });

  // ---------- тема ----------
  var savedTheme = store("theme");
  if (savedTheme === "light" || savedTheme === "dark") document.documentElement.setAttribute("data-theme", savedTheme);
  function currentTheme() {
    var t = document.documentElement.getAttribute("data-theme");
    if (t) return t;
    return window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
  }

  document.addEventListener("DOMContentLoaded", function () {
    // ---------- бокове меню ----------
    var side = document.getElementById("sidebar");
    if (side) {
      var html = '<a class="brand" href="index.html"><span class="brand-mark">μC</span>' +
        '<span class="brand-text">Мікроконтролери з нуля<small>ESP32-C3 · STM32 · LED</small></span></a>';
      var group = "";
      html += '<nav class="nav" aria-label="Розділи">';
      PAGES.forEach(function (p, i) {
        if (p.group !== group) { group = p.group; html += '<div class="nav-group">' + group + "</div>"; }
        html += '<a href="' + p.file + '"' + (i === idx ? ' class="active" aria-current="page"' : "") + '>' +
          '<span class="num">' + p.num + "</span><span>" + p.title + "</span></a>";
        if (i === idx) html += '<ul class="toc" id="page-toc"></ul>';
      });
      html += "</nav>";
      html += '<div class="sidebar-tools"><button class="btn" id="theme-btn" type="button"></button>' +
        '<button class="btn" type="button" onclick="window.print()">Друк / PDF</button></div>';
      side.innerHTML = html;

      // зміст поточної сторінки з h2
      var toc = document.getElementById("page-toc");
      var heads = document.querySelectorAll(".main h2[id]");
      if (toc && heads.length) {
        heads.forEach(function (h) {
          var li = document.createElement("li");
          var a = document.createElement("a");
          a.href = "#" + h.id;
          a.textContent = h.getAttribute("data-short") || h.textContent;
          a.addEventListener("click", function () { document.body.classList.remove("nav-open"); });
          li.appendChild(a); toc.appendChild(li);
        });
      } else if (toc) { toc.remove(); }
    }

    var themeBtn = document.getElementById("theme-btn");
    function paintThemeBtn() { if (themeBtn) themeBtn.textContent = currentTheme() === "dark" ? "☀ Світла тема" : "☾ Темна тема"; }
    if (themeBtn) {
      paintThemeBtn();
      themeBtn.addEventListener("click", function () {
        var next = currentTheme() === "dark" ? "light" : "dark";
        document.documentElement.setAttribute("data-theme", next);
        store("theme", next);
        paintThemeBtn();
      });
    }

    // ---------- мобільне меню ----------
    var menuBtn = document.querySelector(".menu-btn");
    if (menuBtn) menuBtn.addEventListener("click", function () { document.body.classList.toggle("nav-open"); });
    document.addEventListener("click", function (e) {
      if (!document.body.classList.contains("nav-open")) return;
      if (side && !side.contains(e.target) && e.target !== menuBtn) document.body.classList.remove("nav-open");
    });

    // ---------- пейджер ----------
    var pager = document.getElementById("pager");
    if (pager) {
      var prev = PAGES[idx - 1], next = PAGES[idx + 1], ph = "";
      ph += prev ? '<a class="prev" href="' + prev.file + '"><small>← Попередній</small>' + prev.title + "</a>" : "<span></span>";
      ph += next ? '<a class="next" href="' + next.file + '"><small>Наступний →</small>' + next.title + "</a>" : "<span></span>";
      pager.innerHTML = ph;
    }

    // ---------- кнопка «Копіювати» ----------
    document.querySelectorAll("pre").forEach(function (pre) {
      var b = document.createElement("button");
      b.type = "button"; b.className = "copy-btn"; b.textContent = "Копіювати";
      b.addEventListener("click", function () {
        var text = (pre.querySelector("code") || pre).innerText;
        var done = function () { b.textContent = "Скопійовано ✓"; setTimeout(function () { b.textContent = "Копіювати"; }, 1500); };
        if (navigator.clipboard && navigator.clipboard.writeText) {
          navigator.clipboard.writeText(text).then(done, function () { fallback(text); done(); });
        } else { fallback(text); done(); }
      });
      pre.appendChild(b);
    });
    function fallback(text) {
      var ta = document.createElement("textarea");
      ta.value = text; document.body.appendChild(ta); ta.select();
      try { document.execCommand("copy"); } catch (e) { /* ignore */ }
      ta.remove();
    }

    // ---------- чек-листи зі збереженням стану ----------
    document.querySelectorAll(".checklist input[type=checkbox][data-key]").forEach(function (cb) {
      var key = "chk:" + here + ":" + cb.getAttribute("data-key");
      if (store(key) === "1") cb.checked = true;
      cb.addEventListener("change", function () { store(key, cb.checked ? "1" : "0"); });
    });
  });
})();
