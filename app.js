(() => {
  "use strict";
  const year = document.getElementById("year");
  const clock = document.getElementById("clock");
  const date = document.getElementById("date");
  if (year) year.textContent = String(new Date().getFullYear());

  function updateClock() {
    const now = new Date();
    if (clock) clock.textContent = now.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
    if (date) date.textContent = now.toLocaleDateString([], { day: "2-digit", month: "2-digit", year: "numeric" });
  }
  updateClock();
  window.setInterval(updateClock, 30000);

  const navLinks = [...document.querySelectorAll(".menubar a, .sidebar a.side-link[href^='#']")];
  const sections = [...document.querySelectorAll(".main-pane section[id]")];
  const setActive = (id) => {
    navLinks.forEach((link) => {
      const active = link.getAttribute("href") === "#" + id;
      link.classList.toggle("selected", active && link.classList.contains("side-link"));
      if (active && link.classList.contains("side-link")) link.setAttribute("aria-current", "page");
      else link.removeAttribute("aria-current");
    });
  };
  navLinks.forEach((link) => link.addEventListener("click", () => {
    const id = link.getAttribute("href").slice(1);
    setActive(id);
  }));

  if ("IntersectionObserver" in window) {
    const observer = new IntersectionObserver((entries) => {
      const visible = entries.filter((entry) => entry.isIntersecting).sort((a, b) => b.intersectionRatio - a.intersectionRatio)[0];
      if (visible) setActive(visible.target.id);
    }, { rootMargin: "-15% 0px -65% 0px", threshold: [0, .15, .4] });
    sections.forEach((section) => observer.observe(section));
  }
})();