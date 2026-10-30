(() => {
  "use strict";
  const year=document.getElementById("year"),clock=document.getElementById("clock"),date=document.getElementById("date");
  if(year)year.textContent=String(new Date().getFullYear());
  function tick(){const n=new Date();if(clock)clock.textContent=n.toLocaleTimeString([],{hour:"2-digit",minute:"2-digit"});if(date)date.textContent=n.toLocaleDateString([],{day:"2-digit",month:"2-digit",year:"numeric"});}
  tick();window.setInterval(tick,30000);
  const links=[...document.querySelectorAll(".menu-bar a,.nav-item[href^='#']")],sections=[...document.querySelectorAll(".main-content section[id]")];
  function activate(id){links.forEach(a=>{const on=a.getAttribute("href")==="#"+id;if(a.classList.contains("nav-item"))a.classList.toggle("active",on);if(on&&a.classList.contains("nav-item"))a.setAttribute("aria-current","page");else a.removeAttribute("aria-current");});}
  links.forEach(a=>a.addEventListener("click",()=>activate(a.getAttribute("href").slice(1))));
  if("IntersectionObserver"in window){const observer=new IntersectionObserver(entries=>{const current=entries.filter(e=>e.isIntersecting).sort((a,b)=>b.intersectionRatio-a.intersectionRatio)[0];if(current)activate(current.target.id);},{rootMargin:"-15% 0px -65% 0px",threshold:[0,.15,.4]});sections.forEach(s=>observer.observe(s));}
})();