/* site.js — Subtivals vanilla JS, no dependencies */
'use strict';

/* ---- Preloader -------------------------------------------- */
window.addEventListener('load', () => {
  const el = document.querySelector('.preloader');
  if (!el) return;
  el.classList.add('fade-out');
  el.addEventListener('transitionend', () => el.remove(), { once: true });
});

/* ---- Init ------------------------------------------------- */
document.addEventListener('DOMContentLoaded', () => {
  initNav();
  initTestimonials();
  initSnapCarousel('snap-screenshots');
  initSnapCarousel('snap-insitu');
  initLightbox();
  initReveal();
});

/* ---- Navigation ------------------------------------------- */
function initNav() {
  const nav = document.querySelector('.site-nav');
  if (!nav) return;

  // Shrink on scroll
  window.addEventListener('scroll', () => {
    nav.classList.toggle('shrunk', window.scrollY > 60);
  }, { passive: true });

  // Smooth scroll for anchor links
  nav.querySelectorAll('a[href^="#"]').forEach(link => {
    link.addEventListener('click', e => {
      const target = document.querySelector(link.getAttribute('href'));
      if (!target) return;
      e.preventDefault();
      target.scrollIntoView({ behavior: 'smooth' });
      // Close mobile menu if open
      const links = nav.querySelector('.nav-links');
      links?.classList.remove('open');
    });
  });

  // Mobile toggle
  const toggle = nav.querySelector('.nav-toggle');
  const links  = nav.querySelector('.nav-links');
  toggle?.addEventListener('click', () => links?.classList.toggle('open'));
}

/* ---- Testimonial carousel --------------------------------- */
function initTestimonials() {
  const wrap = document.querySelector('.quote-carousel');
  if (!wrap) return;

  const items = Array.from(wrap.querySelectorAll('.quote-item'));
  const dots  = Array.from(wrap.querySelectorAll('.carousel-dots button'));
  let cur = 0, timer;

  function show(n) {
    items[cur].classList.remove('active');
    dots[cur]?.classList.remove('active');
    cur = ((n % items.length) + items.length) % items.length;
    items[cur].classList.add('active');
    dots[cur]?.classList.add('active');
  }

  function restart() {
    clearInterval(timer);
    timer = setInterval(() => show(cur + 1), 5000);
  }

  wrap.querySelector('.prev')?.addEventListener('click', () => { show(cur - 1); restart(); });
  wrap.querySelector('.next')?.addEventListener('click', () => { show(cur + 1); restart(); });
  dots.forEach((d, i) => d.addEventListener('click', () => { show(i); restart(); }));

  restart();
}

/* ---- Scroll-snap carousel --------------------------------- */
function initSnapCarousel(id) {
  const wrap = document.getElementById(id);
  if (!wrap) return;
  const track = wrap.querySelector('.snap-track');
  const prev  = wrap.querySelector('.snap-prev');
  const next  = wrap.querySelector('.snap-next');
  const step  = () => track.clientWidth / 3;
  prev?.addEventListener('click', () => track.scrollBy({ left: -step(), behavior: 'smooth' }));
  next?.addEventListener('click', () => track.scrollBy({ left:  step(), behavior: 'smooth' }));
}

/* ---- Lightbox --------------------------------------------- */
function initLightbox() {
  const triggers = Array.from(document.querySelectorAll('.gallery-link'));
  if (!triggers.length) return;

  const dialog = document.createElement('dialog');
  dialog.className = 'lightbox';
  dialog.innerHTML = `
    <div class="lightbox-inner">
      <button class="lightbox-close" aria-label="Close">&times;</button>
      <button class="lb-prev" hidden>&#8249;</button>
      <img class="lightbox-img" src="" alt="">
      <button class="lb-next" hidden>&#8250;</button>
    </div>`;
  document.body.appendChild(dialog);

  const img     = dialog.querySelector('.lightbox-img');
  const btnPrev = dialog.querySelector('.lb-prev');
  const btnNext = dialog.querySelector('.lb-next');
  let group = [], idx = 0;

  // Group links by nearest carousel wrapper
  const galleries = new Map();
  triggers.forEach(t => {
    const key = t.closest('[data-gallery]')?.dataset.gallery ?? 'default';
    if (!galleries.has(key)) galleries.set(key, []);
    galleries.get(key).push(t);
  });

  function open(g, i) {
    group = g; idx = i;
    img.src  = '';
    img.src  = g[i].getAttribute('href');
    img.alt  = g[i].querySelector('img')?.alt ?? '';
    btnPrev.hidden = g.length < 2;
    btnNext.hidden = g.length < 2;
  }

  triggers.forEach(t => {
    t.addEventListener('click', e => {
      e.preventDefault();
      const key = t.closest('[data-gallery]')?.dataset.gallery ?? 'default';
      const g   = galleries.get(key);
      open(g, g.indexOf(t));
      dialog.showModal();
    });
  });

  dialog.querySelector('.lightbox-close').addEventListener('click', () => dialog.close());
  btnPrev.addEventListener('click', () => open(group, (idx - 1 + group.length) % group.length));
  btnNext.addEventListener('click', () => open(group, (idx + 1) % group.length));
  dialog.addEventListener('click', e => { if (e.target === dialog) dialog.close(); });
  dialog.addEventListener('keydown', e => {
    if (e.key === 'ArrowLeft')  btnPrev.click();
    if (e.key === 'ArrowRight') btnNext.click();
  });
}

/* ---- Scroll reveal ---------------------------------------- */
function initReveal() {
  const els = document.querySelectorAll('.reveal');
  if (!els.length) return;
  const obs = new IntersectionObserver(entries => {
    entries.forEach(e => {
      if (e.isIntersecting) { e.target.classList.add('in'); obs.unobserve(e.target); }
    });
  }, { threshold: 0.12 });
  els.forEach(el => obs.observe(el));
}
