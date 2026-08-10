/* ==========================================================================
   Утилизация КНТ — вся логика приложения.
   Ничего не отправляется наружу: файлы читаются и остаются в браузере.
   ========================================================================== */
(function () {
'use strict';

var STORE_KEY = 'utilization-knt/v1';
var $ = function (id) { return document.getElementById(id); };

/* ---------------------------------------------------------------- утилиты */

function digits(v) {
  return String(v == null ? '' : v).replace(/\D+/g, '');
}

/** Нормализация заголовка: регистр, ё→е, лишние пробелы. «№» сохраняем —
    по нему отличается полный «№ КНТ» от короткого «КНТ». */
function normHeader(v) {
  return String(v == null ? '' : v)
    .replace(/ /g, ' ')
    .trim()
    .toLowerCase()
    .replace(/ё/g, 'е')
    .replace(/\s+/g, ' ')
    .replace(/[:.]+$/, '');
}

/** Текст ячейки для показа и выгрузки. */
function cellText(v) {
  if (v == null || v === '') return '';
  if (v instanceof Date) {
    var p = function (n) { return (n < 10 ? '0' : '') + n; };
    return p(v.getDate()) + '.' + p(v.getMonth() + 1) + '.' + v.getFullYear();
  }
  if (typeof v === 'number') {
    if (Number.isInteger(v)) return String(v);
    return String(Math.round(v * 100) / 100);
  }
  return String(v).replace(/ /g, ' ').trim();
}

function pad2(n) { return (n < 10 ? '0' : '') + n; }

/** «1 строка / 2 строки / 5 строк» */
function plural(n, one, few, many) {
  var a = Math.abs(n) % 100, b = a % 10;
  if (a > 10 && a < 20) return n + ' ' + many;
  if (b > 1 && b < 5) return n + ' ' + few;
  if (b === 1) return n + ' ' + one;
  return n + ' ' + many;
}

function hhmmss(ms) {
  var s = Math.max(0, Math.floor(ms / 1000));
  return pad2(Math.floor(s / 3600)) + ':' + pad2(Math.floor(s / 60) % 60) + ':' + pad2(s % 60);
}

function clockOf(ts) {
  var d = new Date(ts);
  return pad2(d.getHours()) + ':' + pad2(d.getMinutes()) + ':' + pad2(d.getSeconds());
}

function stampOf(ts) {
  var d = new Date(ts);
  return pad2(d.getDate()) + '.' + pad2(d.getMonth() + 1) + '.' + d.getFullYear() +
         ' ' + pad2(d.getHours()) + ':' + pad2(d.getMinutes()) + ':' + pad2(d.getSeconds());
}

function esc(s) {
  return String(s == null ? '' : s)
    .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

var snackTimer = null;
function snack(text, kind) {
  var el = $('snack');
  el.className = 'snack' + (kind ? ' ' + kind : '');
  el.textContent = text;
  el.hidden = false;
  clearTimeout(snackTimer);
  snackTimer = setTimeout(function () { el.hidden = true; }, 2600);
}

function copyText(text, okMsg) {
  var done = function () { snack(okMsg || 'Скопировано', 'ok'); };
  // execCommand работает только внутри жеста пользователя: если мы сюда попали
  // по таймауту, он уже бесполезен — показываем текст для ручного копирования
  var fallback = function (sync) {
    if (sync && legacyCopy(text)) { done(); return; }
    manualCopy(text);
  };

  if (!navigator.clipboard || !navigator.clipboard.writeText) { fallback(true); return; }

  // writeText умеет зависать, ожидая разрешения (старые WebView, автоматизация)
  var settled = false;
  var timer = setTimeout(function () {
    if (settled) return;
    settled = true;
    fallback(false);
  }, 1200);
  navigator.clipboard.writeText(text).then(function () {
    if (settled) return;
    settled = true; clearTimeout(timer); done();
  }, function () {
    if (settled) return;
    settled = true; clearTimeout(timer); fallback(false);
  });
}

/** Последний рубеж: показать текст выделенным, чтобы его скопировали руками. */
function manualCopy(text) {
  $('modal-t').textContent = 'Скопируйте текст вручную';
  $('modal-b').innerHTML =
    '<textarea class="inp mono" id="manual-copy" readonly rows="8"></textarea>' +
    '<div class="hint">Браузер не дал доступ к буферу обмена. Текст уже выделен — ' +
    'скопируйте его долгим нажатием.</div>';
  $('manual-copy').value = text;
  $('modal-yes').textContent = 'Готово';
  $('modal-no').hidden = true;
  $('modal').hidden = false;

  var ta = $('manual-copy');
  ta.focus();
  ta.setSelectionRange(0, text.length);

  var close = function () {
    $('modal').hidden = true;
    $('modal-no').hidden = false;
    $('modal-yes').onclick = null;
    $('modal').onclick = null;
  };
  $('modal-yes').onclick = close;
  $('modal').onclick = function (e) { if (e.target === $('modal')) close(); };
}

function legacyCopy(text) {
  var ta = document.createElement('textarea');
  ta.value = text;
  ta.setAttribute('readonly', '');
  ta.style.cssText = 'position:fixed;top:-1000px;left:0;opacity:0';
  document.body.appendChild(ta);
  ta.select();
  ta.setSelectionRange(0, ta.value.length);
  var ok = false;
  try { ok = document.execCommand('copy'); } catch (e) { ok = false; }
  document.body.removeChild(ta);
  return ok;
}

/** Диалог подтверждения. Возвращает промис с true/false. */
function confirmBox(title, body, yesLabel) {
  return new Promise(function (resolve) {
    $('modal-t').textContent = title;
    $('modal-b').textContent = body;          // заодно сбрасывает разметку ручного копирования
    $('modal-yes').textContent = yesLabel || 'Да';
    $('modal-no').hidden = false;
    $('modal').hidden = false;
    var close = function (v) {
      $('modal').hidden = true;
      $('modal-yes').onclick = null;
      $('modal-no').onclick = null;
      $('modal').onclick = null;
      resolve(v);
    };
    $('modal-yes').onclick = function () { close(true); };
    $('modal-no').onclick = function () { close(false); };
    $('modal').onclick = function (e) { if (e.target === $('modal')) close(false); };
  });
}

function showScreen(id) {
  ['screen-load', 'screen-block', 'screen-work', 'screen-done'].forEach(function (s) {
    $(s).classList.toggle('on', s === id);
  });
  window.scrollTo(0, 0);
}

/* ------------------------------------------------------------- чтение книг */

var ALIAS_KNT_FULL = ['№ кнт', '№кнт', 'no кнт', 'n кнт', '# кнт', 'номер кнт', 'номер кнт (полный)'];
var ALIAS_KNT_SHORT = ['кнт'];
var ALIAS_DECISION = ['решение', 'решение комиссии'];
var ALIAS_NAME = ['наименование товара', 'наименование', 'товар'];

function isCsvName(name) { return /\.(csv|txt)$/i.test(name || ''); }

/** Читает файл в матрицу строк. Понимает xlsx/xlsm/xls и csv (в т.ч. windows-1251). */
function readWorkbook(file) {
  return new Promise(function (resolve, reject) {
    var fr = new FileReader();
    fr.onerror = function () { reject(new Error('Файл не читается')); };
    fr.onload = function () {
      try {
        var wb;
        if (isCsvName(file.name)) {
          var buf = fr.result;
          var text = new TextDecoder('utf-8').decode(buf);
          if (text.indexOf('�') >= 0) text = new TextDecoder('windows-1251').decode(buf);
          var head = text.split(/\r?\n/)[0] || '';
          var semi = (head.match(/;/g) || []).length;
          var comma = (head.match(/,/g) || []).length;
          var tab = (head.match(/\t/g) || []).length;
          var FS = tab > semi && tab > comma ? '\t' : (semi >= comma ? ';' : ',');
          wb = XLSX.read(text, { type: 'string', FS: FS, cellDates: true, raw: true });
        } else {
          wb = XLSX.read(new Uint8Array(fr.result), { type: 'array', cellDates: true });
        }
        var sheetName = wb.SheetNames[0];
        var ws = wb.Sheets[sheetName];
        if (!ws) throw new Error('В книге нет ни одного листа');
        var rows = XLSX.utils.sheet_to_json(ws, { header: 1, raw: true, defval: '', blankrows: false });
        resolve({ rows: rows, sheet: sheetName });
      } catch (e) {
        reject(new Error('Не удалось разобрать файл: ' + (e && e.message ? e.message : e)));
      }
    };
    fr.readAsArrayBuffer(file);
  });
}

/** Ищет строку шапки в первых 30 строках. need — список наборов синонимов,
    каждый из которых обязан найтись в строке. */
function findHeader(rows, need) {
  var limit = Math.min(rows.length, 30);
  for (var i = 0; i < limit; i++) {
    var norm = (rows[i] || []).map(normHeader);
    var ok = need.every(function (aliases) {
      return norm.some(function (h) { return aliases.indexOf(h) >= 0; });
    });
    if (ok) return { index: i, headers: (rows[i] || []).map(cellText), norm: norm };
  }
  return null;
}

function colOf(norm, aliases) {
  for (var i = 0; i < norm.length; i++) {
    if (aliases.indexOf(norm[i]) >= 0) return i;
  }
  return -1;
}

/** «НЕ ДОПУЩЕН» не должен пройти по подстроке «ДОПУЩЕН» — проверяем начало. */
function isAllowed(value) {
  var up = String(value == null ? '' : value).replace(/ /g, ' ').trim().toUpperCase().replace(/Ё/g, 'Е');
  if (!up) return false;
  // \b в JS не работает с кириллицей (\w — только латиница), поэтому сравниваем
  // строку без пробелов: «НЕ ДОПУЩЕН» и «НЕДОПУЩЕН» должны отсекаться одинаково
  var flat = up.replace(/\s+/g, '');
  if (flat.indexOf('НЕДОПУЩ') === 0) return false;
  if (flat === 'НЕТ') return false;
  return flat.indexOf('ДОПУЩ') >= 0;
}

/* ------------------------------------------------------------- разбор файлов */

var parsedCheck = null;   // { headers, rows[], allowed[], denied[], empty }
var parsedAct = null;     // { knt: [..], dupes: n, rowsCount }

function parseCheckFile(file) {
  return readWorkbook(file).then(function (book) {
    var head = findHeader(book.rows, [ALIAS_DECISION, ALIAS_KNT_FULL.concat(ALIAS_KNT_SHORT)]);
    if (!head) {
      var seen = (book.rows[0] || []).map(cellText).filter(Boolean).join(' · ');
      throw new Error('Не нашёл столбцы «Решение» и «№ КНТ». Заголовки первой строки: ' + (seen || '—'));
    }
    var cDec = colOf(head.norm, ALIAS_DECISION);
    var cKnt = colOf(head.norm, ALIAS_KNT_FULL);
    if (cKnt < 0) cKnt = colOf(head.norm, ALIAS_KNT_SHORT);
    var cShort = -1;
    for (var i = 0; i < head.norm.length; i++) {
      if (i !== cKnt && ALIAS_KNT_SHORT.indexOf(head.norm[i]) >= 0) { cShort = i; break; }
    }
    var cName = colOf(head.norm, ALIAS_NAME);

    var allowed = [], denied = [], empty = 0, noKnt = 0;
    for (var r = head.index + 1; r < book.rows.length; r++) {
      var row = book.rows[r] || [];
      var knt = digits(cellText(row[cKnt]));
      var dec = cellText(row[cDec]);
      if (!knt) {
        if (row.some(function (c) { return cellText(c) !== ''; })) noKnt++;
        continue;
      }
      var cells = [];
      for (var c = 0; c < head.headers.length; c++) cells.push(cellText(row[c]));
      var item = {
        knt: knt,
        short: cShort >= 0 ? digits(cellText(row[cShort])) : knt.slice(-4),
        name: cName >= 0 ? cellText(row[cName]) : '',
        decision: dec,
        cells: cells
      };
      if (isAllowed(dec)) allowed.push(item);
      else { denied.push(item); if (!dec) empty++; }
    }

    return {
      fileName: file.name,
      sheet: book.sheet,
      headers: head.headers,
      headerRow: head.index + 1,
      allowed: allowed,
      denied: denied,
      empty: empty,
      noKnt: noKnt,
      total: allowed.length + denied.length
    };
  });
}

function parseActFile(file) {
  return readWorkbook(file).then(function (book) {
    var head = findHeader(book.rows, [ALIAS_KNT_FULL.concat(ALIAS_KNT_SHORT)]);
    if (!head) {
      var seen = (book.rows[0] || []).map(cellText).filter(Boolean).join(' · ');
      throw new Error('Не нашёл столбец «№ КНТ» в первых 30 строках. Заголовки первой строки: ' + (seen || '—'));
    }
    var cKnt = colOf(head.norm, ALIAS_KNT_FULL);
    if (cKnt < 0) cKnt = colOf(head.norm, ALIAS_KNT_SHORT);

    var seenSet = Object.create(null), list = [], dupes = 0;
    for (var r = head.index + 1; r < book.rows.length; r++) {
      var knt = digits(cellText((book.rows[r] || [])[cKnt]));
      if (!knt) continue;
      if (seenSet[knt]) { dupes++; continue; }
      seenSet[knt] = true;
      list.push(knt);
    }
    if (!list.length) throw new Error('В столбце «№ КНТ» акта нет ни одного номера');

    return { fileName: file.name, sheet: book.sheet, knt: list, dupes: dupes, headerRow: head.index + 1 };
  });
}

/* --------------------------------------------------------------- состояние */

var S = null;      // рабочее состояние (см. blankState)
var tickTimer = null;

function blankState() {
  return {
    v: 1,
    meta: { checkFile: '', actFile: '', createdAt: 0 },
    headers: [],
    items: [],
    marks: {},
    log: [],
    timer: { state: 'idle', startedAt: 0, anchor: 0, accMs: 0, pausedMs: 0, pauseAnchor: 0, pauses: 0, finishedAt: 0 },
    cursor: -1
  };
}

function save() {
  try {
    localStorage.setItem(STORE_KEY, JSON.stringify(S));
  } catch (e) {
    snack('Не удалось сохранить состояние (переполнено хранилище)', 'err');
  }
}

function load() {
  try {
    var raw = localStorage.getItem(STORE_KEY);
    if (!raw) return null;
    var obj = JSON.parse(raw);
    if (!obj || obj.v !== 1 || !Array.isArray(obj.items) || !obj.items.length) return null;
    return obj;
  } catch (e) { return null; }
}

function dropStorage() {
  try { localStorage.removeItem(STORE_KEY); } catch (e) { /* нечего делать */ }
}

/* ------------------------------------------------------------------ таймер */

function tState() { return S.timer.state; }

function elapsedMs() {
  var t = S.timer;
  return t.accMs + (t.state === 'run' ? Date.now() - t.anchor : 0);
}

function pausedMsTotal() {
  var t = S.timer;
  return t.pausedMs + (t.state === 'pause' ? Date.now() - t.pauseAnchor : 0);
}

function timerStart() {
  var t = S.timer, now = Date.now();
  t.state = 'run'; t.startedAt = now; t.anchor = now;
  save(); renderWork();
}

function timerPause() {
  var t = S.timer, now = Date.now();
  if (t.state !== 'run') return;
  t.accMs += now - t.anchor; t.anchor = 0;
  t.pauseAnchor = now; t.pauses++; t.state = 'pause';
  save(); renderWork();
}

function timerResume() {
  var t = S.timer, now = Date.now();
  if (t.state !== 'pause') return;
  t.pausedMs += now - t.pauseAnchor; t.pauseAnchor = 0;
  t.anchor = now; t.state = 'run';
  save(); renderWork();
}

function timerFinish() {
  var t = S.timer, now = Date.now();
  if (t.state === 'run') t.accMs += now - t.anchor;
  if (t.state === 'pause') t.pausedMs += now - t.pauseAnchor;
  t.anchor = 0; t.pauseAnchor = 0; t.state = 'done'; t.finishedAt = now;
  save();
}

/* ----------------------------------------------------------------- сверка */

function runMatch() {
  $('load-error').innerHTML = '';
  if (!parsedCheck || !parsedAct) { $('match-box').hidden = true; $('btn-start').disabled = true; return; }

  var allowedMap = Object.create(null);
  parsedCheck.allowed.forEach(function (it) { allowedMap[it.knt] = it; });
  var deniedMap = Object.create(null);
  parsedCheck.denied.forEach(function (it) { if (!deniedMap[it.knt]) deniedMap[it.knt] = it; });

  var extra = [], inWork = [];
  parsedAct.knt.forEach(function (k) {
    if (allowedMap[k]) inWork.push(allowedMap[k]);
    else if (deniedMap[k]) extra.push({ knt: k, why: 'НЕ ДОПУЩЕН', name: deniedMap[k].name, miss: false });
    else extra.push({ knt: k, why: 'нет в списке проверки', name: '', miss: true });
  });

  if (extra.length) { showBlock(extra, parsedAct.knt.length); return; }

  // выборка сужается до акта
  var kpis = [
    { u: 'строк в файле', b: parsedCheck.total },
    { u: 'допущено', b: parsedCheck.allowed.length, cls: '' },
    { u: 'не допущено', b: parsedCheck.denied.length - parsedCheck.empty, cls: 'bad' },
    { u: 'пустое решение', b: parsedCheck.empty, cls: parsedCheck.empty ? 'bad' : '' },
    { u: 'в акте', b: parsedAct.knt.length },
    { u: 'к утилизации', b: inWork.length, cls: 'acc' }
  ];
  $('match-kpis').innerHTML = kpis.map(function (k) {
    return '<div class="kpi ' + (k.cls || '') + '"><u>' + esc(k.u) + '</u><b>' + k.b + '</b></div>';
  }).join('');

  var notes = [];
  if (inWork.length < parsedCheck.allowed.length) {
    notes.push({ cls: 'warn', t: 'Выборка сокращена по акту',
      s: 'В акте ' + inWork.length + ' из ' + parsedCheck.allowed.length + ' допущенных позиций. Ещё ' +
         plural(parsedCheck.allowed.length - inWork.length, 'позиция', 'позиции', 'позиций') +
         ' в работу не берутся.' });
  } else {
    notes.push({ cls: 'ok', t: 'Акт совпадает со списком допущенных', s: 'Лишнего товара в акте нет.' });
  }
  if (parsedAct.dupes) {
    notes.push({ cls: 'warn', t: 'Дубли в акте', s: 'Повторов номеров: ' + parsedAct.dupes + ' — учтены по одному разу.' });
  }
  if (parsedCheck.empty) {
    notes.push({ cls: 'warn', t: 'Есть непроверенные строки',
      s: 'В файле проверки ' + plural(parsedCheck.empty, 'строка', 'строки', 'строк') +
         ' с пустым «Решением» — они считаются недопущенными.' });
  }
  $('match-notes').innerHTML = notes.map(function (n) {
    return '<div class="toast ' + n.cls + '"><b>' + esc(n.t) + '</b>' + esc(n.s) + '</div>';
  }).join('');

  $('match-box').hidden = false;
  $('btn-start').disabled = false;
  $('btn-start').dataset.count = inWork.length;
  window._inWork = inWork;
}

function showBlock(extra, actTotal) {
  $('btn-start').disabled = true;
  $('match-box').hidden = true;

  var allMissing = extra.length === actTotal && extra.every(function (e) { return e.miss; });
  $('extra-count').textContent = '· ' + extra.length;
  $('extra-list').innerHTML =
    (allMissing ? '<div class="toast warn"><b>Ни один номер из акта не найден в проверке</b>' +
      'Похоже, загружен не тот файл или в акте другой столбец с номерами.</div>' : '') +
    extra.map(function (e) {
      return '<div class="x' + (e.miss ? ' miss' : '') + '"><b>' + esc(e.knt) + '</b>' +
             '<span>' + esc(e.why) + '</span></div>';
    }).join('');

  window._extra = extra;
  showScreen('screen-block');
}

function extraAsText() {
  var extra = window._extra || [];
  var lines = ['Акт списания необходимо исправить: содержит товар, не допущенный к утилизации.',
               'Лишние КНТ (' + extra.length + '):'];
  extra.forEach(function (e) { lines.push(e.knt + ' — ' + e.why); });
  return lines.join('\n');
}

/* ------------------------------------------------------- старт утилизации */

function startSession() {
  var inWork = window._inWork || [];
  if (!inWork.length) return;

  S = blankState();
  S.meta = { checkFile: parsedCheck.fileName, actFile: parsedAct.fileName, createdAt: Date.now() };
  S.headers = parsedCheck.headers;
  S.items = inWork.slice().sort(function (a, b) {
    return a.name.localeCompare(b.name, 'ru') || a.knt.localeCompare(b.knt);
  });
  S.cursor = -1;
  save();
  openWork();
}

function openWork() {
  showScreen('screen-work');
  $('panel-src').textContent = S.meta.actFile || '';
  renderWork();
  startTick();
  focusInput();
}

function startTick() {
  clearInterval(tickTimer);
  tickTimer = setInterval(function () {
    if (!S) return;
    if (S.timer.state === 'run' || S.timer.state === 'pause') renderTimer();
  }, 1000);
}

function focusInput() {
  var el = $('knt-input');
  if (el && !el.disabled) setTimeout(function () { el.focus(); }, 30);
}

/* ------------------------------------------------------------- отрисовка */

function markedCount() { return Object.keys(S.marks).length; }

function renderTimer() {
  var t = S.timer;
  var pill = $('tm-pill');
  var names = { idle: 'не начата', run: 'идёт', pause: 'пауза', done: 'завершена' };
  var cls = { idle: 'stop', run: 'run', pause: 'pause', done: 'done' };
  pill.textContent = names[t.state];
  pill.className = 'pill ' + cls[t.state];
  $('tm-value').textContent = hhmmss(elapsedMs());

  var line = [];
  if (t.startedAt) line.push('старт ' + clockOf(t.startedAt));
  line.push('пауз ' + t.pauses);
  if (t.pauses || t.state === 'pause') line.push('в паузе ' + hhmmss(pausedMsTotal()));
  if (t.finishedAt) line.push('финиш ' + clockOf(t.finishedAt));
  $('tm-line').textContent = line.join(' · ');

  $('btn-tm-start').hidden = t.state !== 'idle';
  $('btn-tm-pause').hidden = t.state !== 'run';
  $('btn-tm-resume').hidden = t.state !== 'pause';
  $('btn-tm-finish').hidden = (t.state === 'idle' || t.state === 'done');
}

function renderProgress() {
  var total = S.items.length, done = markedCount();
  var pct = total ? Math.round(done / total * 100) : 0;
  $('pg-bar').style.width = pct + '%';
  $('pg-line').textContent = done + ' / ' + total + ' · осталось ' + (total - done);
}

function renderLog() {
  var box = $('log-list');
  if (!S.log.length) { box.innerHTML = '<span class="muted">пока пусто</span>'; return; }
  box.innerHTML = S.log.slice(0, 30).map(function (e) {
    return '<div class="l' + (e.type === 'undo' ? ' undo' : '') + '">' +
      '<i>' + clockOf(e.ts) + '</i><s>' + esc(e.knt) + '</s>' +
      '<em>' + esc(e.type === 'undo' ? 'снята отметка · ' + e.name : e.name) + '</em></div>';
  }).join('');
}

function renderCard() {
  var wrap = $('card-wrap');
  if (S.cursor < 0 || !S.items[S.cursor]) { wrap.hidden = true; return; }
  var it = S.items[S.cursor];
  var marked = !!S.marks[it.knt];

  wrap.hidden = false;
  wrap.classList.toggle('marked', marked);
  $('card-stamp').hidden = !marked;

  $('c-knt').textContent = it.knt;
  var sub = ['короткий: ' + (it.short || '—'), 'позиция ' + (S.cursor + 1) + ' из ' + S.items.length];
  if (marked) sub.push('отмечен ' + clockOf(S.marks[it.knt]));
  $('c-knt-sub').textContent = sub.join(' · ');
  $('c-name').textContent = it.name || '(наименование не заполнено)';

  // атрибуты и чипы собираются из исходных столбцов файла
  var ATTRS = ['правило дефектации', 'подгруппа товаров', 'подгруппа', 'торговая марка',
               'код товара', 'цена по каталогу кис', 'цена кис', 'дефектов кол-во',
               'кол-во вложений', 'киз', 'списание согласовано дпп'];
  var CHIPS = ['описание на этикетке', 'место обнаружения нт', 'состояние нт'];
  var attrHtml = '', chipHtml = '';
  S.headers.forEach(function (h, i) {
    var n = normHeader(h), v = it.cells[i];
    if (!v) return;
    if (ATTRS.indexOf(n) >= 0) {
      attrHtml += '<div><span>' + esc(h) + '</span><span>' + esc(v) + '</span></div>';
    } else if (CHIPS.indexOf(n) >= 0) {
      chipHtml += '<span class="chip">' + esc(h) + ': ' + esc(v) + '</span>';
    }
  });
  $('c-attr').innerHTML = attrHtml;
  $('c-chips').innerHTML = chipHtml;

  var running = tState() === 'run';
  $('btn-util').hidden = marked;
  $('btn-unutil').hidden = !marked;
  $('btn-util').disabled = !running;
  $('util-hint').textContent = marked
    ? 'Решение по этой позиции уже принято. Снять отметку можно только через подтверждение.'
    : (running ? '' : (tState() === 'idle' ? 'Сначала нажмите «Начать» — без запущенного таймера отметка недоступна.'
                                           : 'Утилизация на паузе — продолжите процесс, чтобы отмечать товар.'));
}

function renderWork() {
  renderTimer();
  renderProgress();
  renderCard();
  renderLog();
}

/* --------------------------------------------------------------- поиск КНТ */

function candidates(q) {
  var d = digits(q);
  if (!d) return [];
  return S.items.filter(function (it) {
    if (d.length >= it.knt.length) return it.knt === d;
    return it.knt.slice(-d.length) === d || it.short === d;
  });
}

function findMsg(html) { $('find-msg').innerHTML = html; }

function doFind(auto) {
  var input = $('knt-input');
  var q = digits(input.value);
  $('picker').hidden = true;
  input.classList.remove('err');

  if (q.length < 2) {
    if (!auto) { findMsg('<div class="toast err">Введите минимум 2 цифры номера</div>'); input.classList.add('err'); }
    else findMsg('');
    return;
  }

  var found = candidates(q);
  if (!found.length) {
    findMsg('<div class="toast err"><b>' + esc(q) + ' — нет в выборке</b>' +
            'Этого номера нет среди допущенного товара из акта.</div>');
    input.classList.add('err');
    $('card-wrap').hidden = true;
    return;
  }

  if (found.length === 1) {
    findMsg('');
    openItem(S.items.indexOf(found[0]));
    return;
  }

  findMsg('<div class="toast warn"><b>Совпало ' + plural(found.length, 'позиция', 'позиции', 'позиций') +
          '</b>Выберите нужную.</div>');
  $('picker').innerHTML = found.map(function (it) {
    var idx = S.items.indexOf(it), done = !!S.marks[it.knt];
    return '<button type="button" data-idx="' + idx + '" class="' + (done ? 'done' : '') + '">' +
      '<b>' + esc(it.knt) + (done ? ' · утилизирован' : '') + '</b>' +
      '<span>' + esc(it.name) + '</span></button>';
  }).join('');
  $('picker').hidden = false;
}

function openItem(idx) {
  if (idx < 0 || idx >= S.items.length) return;
  S.cursor = idx;
  $('picker').hidden = true;
  findMsg('');
  save();
  renderCard();
  $('card-wrap').scrollIntoView({ block: 'nearest', behavior: 'smooth' });
}

/* -------------------------------------------------------------- отметки */

function markCurrent() {
  var it = S.items[S.cursor];
  if (!it) return;
  if (tState() !== 'run') { snack('Таймер не запущен', 'err'); return; }
  if (S.marks[it.knt]) { snack('Уже утилизирован', 'err'); return; }

  var ts = Date.now();
  S.marks[it.knt] = ts;
  S.log.unshift({ ts: ts, knt: it.knt, name: it.name, type: 'mark' });
  save();
  renderWork();
  snack('Отмечено · ' + (it.name || it.knt).slice(0, 46), 'ok');

  $('knt-input').value = '';
  focusInput();

  if (markedCount() === S.items.length) {
    snack('Весь товар из акта утилизирован — можно завершать', 'ok');
  }
}

function unmarkCurrent() {
  var it = S.items[S.cursor];
  if (!it || !S.marks[it.knt]) return;
  confirmBox('Снять отметку?',
    'КНТ ' + it.knt + ' перестанет считаться утилизированным. Снятие попадёт в журнал и в итоговый отчёт.',
    'Снять').then(function (yes) {
      if (!yes) return;
      delete S.marks[it.knt];
      S.log.unshift({ ts: Date.now(), knt: it.knt, name: it.name, type: 'undo' });
      save();
      renderWork();
      snack('Отметка снята', 'ok');
    });
}

function step(delta) {
  if (!S.items.length) return;
  var i = S.cursor < 0 ? (delta > 0 ? -1 : 0) : S.cursor;
  openItem((i + delta + S.items.length) % S.items.length);
}

function jumpToTodo() {
  var n = S.items.length;
  if (!n) return;
  for (var k = 1; k <= n; k++) {
    var i = ((S.cursor < 0 ? -1 : S.cursor) + k + n) % n;
    if (!S.marks[S.items[i].knt]) { openItem(i); return; }
  }
  snack('Неутилизированных позиций не осталось', 'ok');
}

/* ------------------------------------------------------------------ итог */

function finish() {
  var left = S.items.length - markedCount();
  var body = left
    ? 'Не отмечено позиций: ' + left + '. Таймер остановится, вернуться к отметкам будет нельзя.'
    : 'Весь товар отмечен. Таймер остановится.';
  confirmBox('Завершить утилизацию?', body, 'Завершить').then(function (yes) {
    if (!yes) return;
    timerFinish();
    openDone();
  });
}

function openDone() {
  var total = S.items.length, done = markedCount(), left = total - done;
  var kpis = [
    { u: 'к утилизации', b: total },
    { u: 'утилизировано', b: done },
    { u: 'осталось', b: left, cls: left ? 'bad' : '' },
    { u: 'чистое время', b: hhmmss(elapsedMs()) },
    { u: 'пауз', b: S.timer.pauses },
    { u: 'в паузе', b: hhmmss(pausedMsTotal()) }
  ];
  $('done-kpis').innerHTML = kpis.map(function (k) {
    return '<div class="kpi ' + (k.cls || '') + '"><u>' + esc(k.u) + '</u><b>' + esc(k.b) + '</b></div>';
  }).join('');

  var rest = S.items.filter(function (it) { return !S.marks[it.knt]; });
  $('done-rest-box').hidden = !rest.length;
  $('done-rest-count').textContent = '· ' + rest.length;
  $('done-rest').innerHTML = rest.map(function (it) {
    return '<div class="x"><b>' + esc(it.knt) + '</b><span>' + esc(it.name.slice(0, 60)) + '</span></div>';
  }).join('');

  var undos = S.log.filter(function (e) { return e.type === 'undo'; });
  $('done-undo-box').hidden = !undos.length;
  $('done-undo').innerHTML = undos.map(function (e) {
    return '<div class="l undo"><i>' + stampOf(e.ts) + '</i><s>' + esc(e.knt) + '</s><em>' + esc(e.name) + '</em></div>';
  }).join('');

  showScreen('screen-done');
}

function doneAsText() {
  var total = S.items.length, done = markedCount();
  var rest = S.items.filter(function (it) { return !S.marks[it.knt]; });
  var lines = [
    'Утилизация КНТ · ' + stampOf(S.timer.finishedAt || Date.now()),
    'Акт: ' + S.meta.actFile,
    'Утилизировано: ' + done + ' из ' + total,
    'Чистое время: ' + hhmmss(elapsedMs()) + ' · пауз ' + S.timer.pauses + ' · в паузе ' + hhmmss(pausedMsTotal())
  ];
  if (rest.length) {
    lines.push('', 'Не отмечено (' + rest.length + '):');
    rest.forEach(function (it) { lines.push(it.knt + ' — ' + it.name); });
  }
  return lines.join('\n');
}

function restText() {
  var rest = S.items.filter(function (it) { return !S.marks[it.knt]; });
  return ['Не утилизировано (' + rest.length + '):']
    .concat(rest.map(function (it) { return it.knt + ' — ' + it.name; })).join('\n');
}

function exportXlsx() {
  var headers = S.headers.concat(['Утилизирован', 'Время отметки']);
  var aoa = [headers];
  S.items.forEach(function (it) {
    var ts = S.marks[it.knt];
    aoa.push(it.cells.concat([ts ? '+' : '', ts ? stampOf(ts) : '']));
  });

  var wb = XLSX.utils.book_new();
  XLSX.utils.book_append_sheet(wb, XLSX.utils.aoa_to_sheet(aoa), 'Утилизация');

  var sum = [
    ['Показатель', 'Значение'],
    ['Файл проверки', S.meta.checkFile],
    ['Акт списания', S.meta.actFile],
    ['К утилизации, позиций', S.items.length],
    ['Утилизировано', markedCount()],
    ['Не отмечено', S.items.length - markedCount()],
    ['Начало', S.timer.startedAt ? stampOf(S.timer.startedAt) : ''],
    ['Окончание', S.timer.finishedAt ? stampOf(S.timer.finishedAt) : ''],
    ['Чистое время', hhmmss(elapsedMs())],
    ['Пауз', S.timer.pauses],
    ['Время в паузе', hhmmss(pausedMsTotal())],
    []
  ];
  var undos = S.log.filter(function (e) { return e.type === 'undo'; });
  if (undos.length) {
    sum.push(['Снятые отметки', '']);
    sum.push(['Время', '№ КНТ', 'Наименование']);
    undos.forEach(function (e) { sum.push([stampOf(e.ts), e.knt, e.name]); });
  }
  XLSX.utils.book_append_sheet(wb, XLSX.utils.aoa_to_sheet(sum), 'Итог');

  var d = new Date();
  var name = 'Утилизация_КНТ_' + pad2(d.getDate()) + '_' + pad2(d.getMonth() + 1) + '_' + d.getFullYear() + '.xlsx';
  XLSX.writeFile(wb, name);
  snack('Файл выгружен: ' + name, 'ok');
}

function restart() {
  confirmBox('Начать заново?', 'Текущая выборка, отметки и таймер будут удалены. Отменить это нельзя.', 'Начать заново')
    .then(function (yes) {
      if (!yes) return;
      dropStorage();
      location.reload();
    });
}

/* ------------------------------------------------------------- обработчики */

function bindFile(inputId, kind) {
  $(inputId).addEventListener('change', function (e) {
    var file = e.target.files && e.target.files[0];
    e.target.value = '';
    if (!file) return;
    var isCheck = kind === 'check';
    var stateEl = $(isCheck ? 'state-check' : 'state-act');
    var dropEl = $(isCheck ? 'drop-check' : 'drop-act');
    stateEl.textContent = 'читаю ' + file.name + '…';
    if (dropEl) dropEl.className = 'drop';

    (isCheck ? parseCheckFile(file) : parseActFile(file)).then(function (res) {
      if (isCheck) {
        parsedCheck = res;
        stateEl.textContent = res.fileName + ' · лист «' + res.sheet + '» · строк ' + res.total +
          ' · допущено ' + res.allowed.length;
      } else {
        parsedAct = res;
        stateEl.textContent = res.fileName + ' · лист «' + res.sheet + '» · номеров ' + res.knt.length;
      }
      if (dropEl) dropEl.className = 'drop ready';
      if (!$('screen-load').classList.contains('on')) showScreen('screen-load');
      runMatch();
    }, function (err) {
      if (isCheck) parsedCheck = null; else parsedAct = null;
      stateEl.textContent = file.name + ' — ошибка';
      if (dropEl) dropEl.className = 'drop err';
      $('match-box').hidden = true;
      $('btn-start').disabled = true;
      $('load-error').innerHTML = '<div class="toast err"><b>' + esc(file.name) + '</b>' + esc(err.message) + '</div>';
      showScreen('screen-load');
    });
  });
}

function bind() {
  bindFile('file-check', 'check');
  bindFile('file-act', 'act');
  bindFile('file-act2', 'act');

  $('btn-start').addEventListener('click', startSession);
  $('btn-copy-extra').addEventListener('click', function () {
    copyText(extraAsText(), 'Список лишних КНТ скопирован');
  });
  $('btn-block-restart').addEventListener('click', function () {
    parsedAct = null;
    $('state-act').textContent = 'файл не выбран';
    $('drop-act').className = 'drop';
    $('match-box').hidden = true;
    $('btn-start').disabled = true;
    showScreen('screen-load');
  });

  $('btn-tm-start').addEventListener('click', function () { timerStart(); focusInput(); });
  $('btn-tm-pause').addEventListener('click', timerPause);
  $('btn-tm-resume').addEventListener('click', function () { timerResume(); focusInput(); });
  $('btn-tm-finish').addEventListener('click', finish);

  var input = $('knt-input');
  input.addEventListener('input', function () {
    var d = digits(input.value);
    if (input.value !== d) input.value = d;
    input.classList.remove('err');
    findMsg('');
    $('picker').hidden = true;
    if (d.length === 4) doFind(true);          // автопоиск на 4-й цифре
  });
  input.addEventListener('keydown', function (e) {
    if (e.key === 'Enter') { e.preventDefault(); doFind(false); }
  });
  $('btn-find').addEventListener('click', function () { doFind(false); });

  $('picker').addEventListener('click', function (e) {
    var btn = e.target.closest('button[data-idx]');
    if (btn) { $('knt-input').value = ''; openItem(parseInt(btn.dataset.idx, 10)); }
  });

  $('btn-util').addEventListener('click', markCurrent);
  $('btn-unutil').addEventListener('click', unmarkCurrent);
  $('btn-prev').addEventListener('click', function () { step(-1); });
  $('btn-next').addEventListener('click', function () { step(1); });
  $('btn-next-todo').addEventListener('click', jumpToTodo);

  $('btn-export').addEventListener('click', exportXlsx);
  $('btn-copy-done').addEventListener('click', function () { copyText(doneAsText(), 'Итог скопирован'); });
  $('btn-copy-rest').addEventListener('click', function () { copyText(restText(), 'Список скопирован'); });
  $('btn-restart').addEventListener('click', restart);

  $('btn-resume').addEventListener('click', function () {
    $('resume-box').hidden = true;
    if (S.timer.state === 'done') openDone(); else openWork();
  });
  $('btn-resume-drop').addEventListener('click', restart);

  // вкладка вернулась из фона — время считается от Date.now(), просто перерисуем
  document.addEventListener('visibilitychange', function () {
    if (!document.hidden && S && $('screen-work').classList.contains('on')) renderWork();
  });

  window.addEventListener('beforeunload', function (e) {
    if (S && S.timer.state === 'run') { e.preventDefault(); e.returnValue = ''; }
  });
}

/* -------------------------------------------------------------------- старт */

function init() {
  if (typeof XLSX === 'undefined') {
    $('load-error').innerHTML = '<div class="toast err"><b>Не загружена библиотека чтения Excel</b>' +
      'Файл vendor/xlsx.full.min.js отсутствует — положите его в репозиторий.</div>';
  }
  bind();

  var saved = load();
  if (saved) {
    S = saved;
    var done = Object.keys(S.marks || {}).length;
    $('resume-info').textContent = 'Акт «' + (S.meta.actFile || '—') + '», отмечено ' + done +
      ' из ' + S.items.length + ', таймер: ' + hhmmss(
        (S.timer.accMs || 0) + (S.timer.state === 'run' ? Date.now() - S.timer.anchor : 0));
    $('resume-box').hidden = false;
    startTick();
  }

  // ?nosw — аварийный выключатель офлайн-кэша: открывает свежую версию в обход
  // service worker'а (нужен при отладке и если в кэш попала сломанная сборка)
  var noSW = location.search.indexOf('nosw') >= 0;
  if ('serviceWorker' in navigator && location.protocol.indexOf('http') === 0 && !noSW) {
    window.addEventListener('load', function () {
      navigator.serviceWorker.register('./sw.js').catch(function () { /* офлайн-режим просто не включится */ });
    });
  }
}

document.addEventListener('DOMContentLoaded', init);
})();
