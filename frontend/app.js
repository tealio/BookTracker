// === Configuration & Globals ===
const apiBase = '/api/books';
let currentSessions = {};   // bookId → sessionId
let overviewChart;          // Chart.js instance

// === Utility Functions ===
// Get current date-time in ISO format
function nowISO() {
    return new Date().toISOString().split('.')[0] + 'Z';
}

// Compute progress percent
function computeProgress(book) {
    return book.totalPages > 0
    ? Math.round((book.pagesRead / book.totalPages) * 100)
    : 0;
}

// Toggle dark mode
function toggleDarkMode() {
    document.documentElement.classList.toggle('dark');
    localStorage.setItem(
        'darkMode',
        document.documentElement.classList.contains('dark') ? 'on' : 'off'
    );
}

// === Authentication ===
async function checkAuth() {
    const res = await fetch('/api/me', {
        credentials: 'include'
    });
    if (res.ok) {
        document.getElementById('auth').style.display = 'none';
        document.getElementById('app').style.display = 'block';
        initApp();
    } else {
        document.getElementById('auth').style.display = 'block';
        document.getElementById('app').style.display = 'none';
    }
}

// Sign up
document.getElementById('signupForm').addEventListener('submit', async e => {
    e.preventDefault();
    const u = document.getElementById('signupUsername').value;
    const p = document.getElementById('signupPassword').value;
    const res = await fetch('/api/signup', {
        method:      'POST',
        credentials: 'include',
        headers:     { 'Content-Type': 'application/json' },
        body:        JSON.stringify({ username: u, password: p })
    });
    if (res.ok) {
        alert('Signed up! Please log in.');
    } else {
        alert((await res.json()).error || 'Signup failed');
    }
});

// Log in
document.getElementById('loginForm').addEventListener('submit', async e => {
    e.preventDefault();
    const u = document.getElementById('loginUsername').value;
    const p = document.getElementById('loginPassword').value;

    console.log('→ POST /api/login', { username: u });
    const res = await fetch('/api/login', {
        method:      'POST',
        credentials: 'include',
        headers:     { 'Content-Type': 'application/json' },
        body:        JSON.stringify({ username: u, password: p })
    });

    console.log('← /api/login status:', res.status);
    console.log('← /api/login headers:', [...res.headers.entries()]);
    console.log('← document.cookie after login:', document.cookie);

    if (res.ok) {
        console.log('Login OK → checking /api/me');
        const me = await fetch('/api/me', { credentials: 'include' });
        console.log('← /api/me status:', me.status, 'body:', await me.text());
        if (me.ok) {
            console.log('Authenticated! Showing app.');
            document.getElementById('auth').style.display = 'none';
            document.getElementById('app').style.display  = 'block';
            initApp();
        } else {
            console.warn('/api/me still 401, stay on login');
            document.getElementById('auth').style.display = 'block';
            document.getElementById('app').style.display  = 'none';
        }
    } else {
        console.warn('Login failed:', await res.text());
        alert((await res.json()).error || 'Login failed');
    }
});


// Log out
document.getElementById('logoutBtn').addEventListener('click', async () => {
    await fetch('/api/logout', {
        method:      'POST',
        credentials: 'include'
    });
    await checkAuth();
});

// === App Initialization ===
function initApp() {
    // Apply dark mode if set
    if (localStorage.getItem('darkMode') === 'on') {
        document.documentElement.classList.add('dark');
    }

    loadBooks();
    loadSessionsAndRenderOverview();

    // Wire up forms & controls
    document.getElementById('searchForm').addEventListener('submit', searchBooks);
    document.getElementById('addBookForm').addEventListener('submit', addBook);
    document.getElementById('filter').addEventListener('change', loadBooks);
    document.getElementById('sort').addEventListener('change', loadBooks);
    document.getElementById('progressSlider').addEventListener('input', () => {
        const tot = parseInt(document.getElementById('totalPages').value) || 0;
        const val = document.getElementById('progressSlider').value;
        document.getElementById('percentLabel').textContent = val + '%';
        document.getElementById('pagesRead').value = Math.round(val * tot / 100);
    });
}

// === Render Stats Dashboard ===
function renderStats(books) {
    const s = document.getElementById('stats');
    const total      = books.length;
    const completed  = books.filter(b => b.status === 'Completed').length;
    const reading    = books.filter(b => b.status === 'Reading').length;
    const notStarted = books.filter(b => b.status === 'Not Started').length;
    const ratings    = books.map(b => b.rating || 0).filter(r => r > 0);
    const avgRating  = ratings.length
    ? (ratings.reduce((a, b) => a + b, 0) / ratings.length).toFixed(1)
    : '—';
    const genreCounts = {};
    books.forEach(b => {
        if (b.genre) genreCounts[b.genre] = (genreCounts[b.genre] || 0) + 1;
    });
        const topGenre = Object.entries(genreCounts)
        .sort((a, b) => b[1] - a[1])[0]?.[0] || '—';

        s.innerHTML = `
        <div><strong>Total Books:</strong> ${total}</div>
        <div><strong>Completed:</strong> ${completed}</div>
        <div><strong>Reading:</strong> ${reading}</div>
        <div><strong>Not Started:</strong> ${notStarted}</div>
        <div><strong>Avg. Rating:</strong> ${avgRating} ⭐️</div>
        <div><strong>Top Genre:</strong> ${topGenre}</div>
        `;
}

// === Load & Render Books ===
async function loadBooks() {
    const res = await fetch(apiBase, {
        credentials: 'include'
    });
    let books = await res.json();
    renderStats(books);

    const f = document.getElementById('filter').value;
    books = books.filter(b => {
        if (f === 'reading')    return b.status === 'Reading';
        if (f === 'completed')  return b.status === 'Completed';
        if (f === 'not-started')return b.status === 'Not Started';
        return true;
    });

    const sv = document.getElementById('sort').value;
    books.sort((a, b) => {
        if (sv === 'title-asc')     return a.title.localeCompare(b.title);
        if (sv === 'title-desc')    return b.title.localeCompare(a.title);
        if (sv === 'progress-asc')  return computeProgress(a) - computeProgress(b);
        if (sv === 'progress-desc') return computeProgress(b) - computeProgress(a);
        if (sv === 'rating-asc')    return a.rating - b.rating;
        if (sv === 'rating-desc')   return b.rating - a.rating;
        return 0;
    });

    const container = document.getElementById('books');
    container.innerHTML = '';

    books.forEach(book => {
        const prog = computeProgress(book);

        const inputC  = 'w-full px-3 py-2 border border-gray-300 rounded ' +
        'bg-white text-gray-900 dark:bg-gray-700 dark:text-white';
        const selectC = inputC + ' mt-1 p-2';

        const tagsI = `<input id="tagsInput-${book.id}" type="text" value="${book.tags}"
        placeholder="Tags" class="${inputC}"/>`;

        const goalI = `<input id="goalEndDateInput-${book.id}" type="date"
        value="${book.goalEndDate}" class="${inputC}"/>`;

        const progressControls = book.status === 'Reading' ? `
        <label>Total Pages:
        <input id="totalPagesInput-${book.id}" type="number" min="0"
        value="${book.totalPages}" onchange="updateBook(${book.id})"
        class="${inputC}"/>
        </label>
        <label>Pages Read:
        <input id="pagesReadInput-${book.id}" type="number" min="0" max="${book.totalPages}"
        value="${book.pagesRead}" onchange="updateBook(${book.id})"
        class="${inputC}"/>
        </label>
        <label>Progress: <span id="progressDisplay-${book.id}">${prog}</span>%</label>
        <input id="progressSlider-${book.id}" type="range" min="0" max="100" value="${prog}"
        oninput="
        const tot = parseInt(document.getElementById('totalPagesInput-${book.id}').value)||0;
        document.getElementById('progressDisplay-${book.id}').textContent=this.value;
        document.getElementById('pagesReadInput-${book.id}').value=Math.round(this.value*tot/100);
        "
        onchange="updateBook(${book.id})"
        class="w-full"/>
        ` : '';

        const sessControls = `
        <div class="mt-2">
        <button id="startBtn-${book.id}"
        onclick="startSession(${book.id})"
        class="px-2 py-1 bg-green-500 text-white rounded">
        Start Session
        </button>
        <button id="stopBtn-${book.id}"
        onclick="stopSession(${book.id})"
        disabled class="px-2 py-1 bg-red-500 text-white rounded">
        Stop Session
        </button>
        <span id="sessionStatus-${book.id}" class="ml-2 text-sm dark:text-gray-300"></span>
        </div>
        `;

        const emojiRating = book.status === 'Completed'
        ? [1,2,3,4,5].map(r => {
            const em = ['😡','😕','😐','😊','🤩'][r-1];
            const sel = (book.rating === r) ? 'scale-125' : 'opacity-50';
            return `<button onclick="setRating(${book.id},${r})"
            class="${sel} hover:scale-110 transition">${em}</button>`;
        }).join('')
        : '';

        const div = document.createElement('div');
        div.id = `book-${book.id}`;
        div.dataset.rating = book.rating;
        div.dataset.thumbnail = book.thumbnail || '';
        div.className = "flex flex-col sm:flex-row gap-4 p-4 bg-white dark:bg-gray-800 rounded shadow";
        div.innerHTML = `
        <div class="flex-shrink-0">
        ${book.thumbnail ? `<img src="${book.thumbnail}" alt="${book.title}" class="h-24 rounded"/>` : ''}
        </div>
        <div class="flex-1 flex flex-col gap-2">
        <div class="flex justify-between items-center">
        <h3 class="text-lg font-bold dark:text-white">${book.title}</h3>
        <button onclick="deleteBook(${book.id})" class="text-red-500">✕</button>
        </div>
        <p><strong>Author:</strong> ${book.author}</p>
        <p><strong>Genre:</strong> ${book.genre || '–'}</p>
        <label>Status:
        <select id="status-${book.id}"
        onchange="updateBook(${book.id})"
        class="${selectC}">
        <option ${book.status==='Not Started'?'selected':''}>Not Started</option>
        <option ${book.status==='Reading'?'selected':''}>Reading</option>
        <option ${book.status==='Completed'?'selected':''}>Completed</option>
        </select>
        </label>
        ${progressControls}
        <label>Tags:</label>${tagsI}
        <label>Goal by:</label>${goalI}
        ${sessControls}
        <label>Notes:</label>
        <textarea id="notes-${book.id}"
        onchange="updateBook(${book.id})"
        class="${inputC}" rows="2">${book.notes}</textarea>
        <div class="flex gap-2 mt-2 text-2xl">${emojiRating}</div>
        </div>
        `;
        container.appendChild(div);
    });
}

// === Add a new book ===
async function addBook(e) {
    e.preventDefault();
    const payload = {
        title:       document.getElementById('title').value,
        author:      document.getElementById('author').value,
        genre:       document.getElementById('genre').value,
        status:      'Not Started',
        pagesRead:   parseInt(document.getElementById('pagesRead').value)  || 0,
        totalPages:  parseInt(document.getElementById('totalPages').value) || 0,
        notes:       '',
        tags:        document.getElementById('tags').value,
        goalEndDate: document.getElementById('goalEndDate').value,
        thumbnail:   document.getElementById('thumbnail').value,
        rating:      3
    };
    await fetch(apiBase, {
        method:      'POST',
        credentials: 'include',
        headers:     { 'Content-Type': 'application/json' },
        body:        JSON.stringify(payload)
    });
    e.target.reset();
    document.getElementById('percentLabel').textContent = '0%';
    await loadBooks();
}

// === Delete a book ===
async function deleteBook(id) {
    if (!confirm("Are you sure you want to delete this book?")) return;
    await fetch(`${apiBase}/${id}`, {
        method:      'DELETE',
        credentials: 'include'
    });
    await loadBooks();
}

// === Update a book ===
async function updateBook(id) {
    const status      = document.getElementById(`status-${id}`).value;
    const notes       = document.getElementById(`notes-${id}`).value;
    const tags        = document.getElementById(`tagsInput-${id}`).value;
    const goalEndDate = document.getElementById(`goalEndDateInput-${id}`).value;
    const tot         = parseInt(document.getElementById(`totalPagesInput-${id}`)?.value) || 0;
    let pagesRead     = 0;
    if (status === 'Reading') {
        pagesRead = parseInt(document.getElementById(`pagesReadInput-${id}`)?.value) || 0;
        pagesRead = Math.min(pagesRead, tot);
    }
    const bookEl    = document.getElementById(`book-${id}`);
    const thumbnail = bookEl.dataset.thumbnail || '';
    const rating    = parseInt(bookEl.dataset.rating, 10) || 3;

    await fetch(`${apiBase}/${id}`, {
        method:      'PUT',
        credentials: 'include',
        headers:     { 'Content-Type': 'application/json' },
        body:        JSON.stringify({ status, pagesRead, totalPages: tot, notes, tags, goalEndDate, thumbnail, rating })
    });
    await loadBooks();
}

// === Change rating ===
async function setRating(id, r) {
    const bookEl = document.getElementById(`book-${id}`);
    bookEl.dataset.rating = r;
    await updateBook(id);
}

// === Reading Sessions ===
async function startSession(bookId) {
    const pages = parseInt(document.getElementById(`pagesReadInput-${bookId}`)?.value) || 0;
    const res = await fetch(`/api/books/${bookId}/session/start`, {
        method:      'POST',
        credentials: 'include',
        headers:     { 'Content-Type': 'application/json' },
        body:        JSON.stringify({ startPagesRead: pages })
    });
    if (res.ok) {
        const { sessionId } = await res.json();
        currentSessions[bookId] = sessionId;
        document.getElementById(`startBtn-${bookId}`).disabled = true;
        document.getElementById(`stopBtn-${bookId}`).disabled  = false;
        document.getElementById(`sessionStatus-${bookId}`).textContent = 'Session started';
    }
}

async function stopSession(bookId) {
    const sessionId = currentSessions[bookId];
    const pages = parseInt(document.getElementById(`pagesReadInput-${bookId}`)?.value) || 0;
    const res = await fetch(`/api/books/${bookId}/session/stop`, {
        method:      'POST',
        credentials: 'include',
        headers:     { 'Content-Type': 'application/json' },
        body:        JSON.stringify({ sessionId, endPagesRead: pages })
    });
    if (res.ok) {
        delete currentSessions[bookId];
        document.getElementById(`startBtn-${bookId}`).disabled = false;
        document.getElementById(`stopBtn-${bookId}`).disabled  = true;
        document.getElementById(`sessionStatus-${bookId}`).textContent = 'Session stopped';
        await loadSessionsAndRenderOverview();
    }
}

// === Load sessions & render overview chart, pace & streak ===
async function loadSessionsAndRenderOverview() {
    const res = await fetch('/api/sessions', {
        credentials: 'include'
    });
    const sessions = await res.json();
    const daily = {};
    sessions.forEach(s => {
        const d = s.startTime.split('T')[0];
        daily[d] = (daily[d] || 0) + s.pagesRead;
    });
    const labels = Object.keys(daily).sort();
    const data   = labels.map(d => daily[d]);

    const ctx = document.getElementById('overviewChart').getContext('2d');
    if (overviewChart) overviewChart.destroy();
    overviewChart = new Chart(ctx, {
        type: 'line',
        data: { labels, datasets: [{ label: 'Pages Read', data, fill: false, tension: 0.1 }] }
    });

    // Pace
    const total = data.reduce((a,b) => a + b, 0);
    const avg   = labels.length ? (total / labels.length).toFixed(1) : 0;
    document.getElementById('paceText').textContent = `Avg pages/day: ${avg}`;

    // Streak
    let streak = 0;
    for (let i = 0; ; i++) {
        const d = new Date(); d.setDate(d.getDate() - i);
        const ds = d.toISOString().split('T')[0];
        if (daily[ds] > 0) streak++;
        else break;
    }
    document.getElementById('streakText').textContent = `Reading streak: ${streak} day${streak === 1 ? '' : 's'}`;
}

// === Search Integration ===
async function searchBooks(e) {
    e.preventDefault();
    const q = encodeURIComponent(document.getElementById('searchInput').value.trim());
    if (!q) return;
    const res = await fetch(`/api/search/${q}`, {
        credentials: 'include'
    });
    const data = await res.json();
    const out  = document.getElementById('searchResults');
    out.innerHTML = '<h3 class="font-semibold dark:text-white">Search Results:</h3>';
    data.items?.slice(0,5).forEach(item => {
        const info = item.volumeInfo;
        const t    = info.title || '';
        const a    = (info.authors || [''])[0];
        const g    = (info.categories || [''])[0];
        const th   = info.imageLinks?.thumbnail || '';
        const div  = document.createElement('div');
        div.className = 'flex gap-4 bg-gray-100 dark:bg-gray-700 p-4 rounded shadow';
        div.innerHTML = `
        ${th ? `<img src="${th}" alt="${t}" class="h-24 rounded"/>` : ''}
        <div class="flex-1">
        <strong class="dark:text-white">${t}</strong>
        <p class="text-sm dark:text-gray-300">${a}</p>
        <button class="bg-primary text-white px-2 py-1 rounded mt-2"
        onclick='addSearchedBook("${escape(t)}","${escape(a)}","${escape(g)}","${escape(th)}")'>
        ➕ Add
        </button>
        </div>`;
        out.appendChild(div);
    });
}

async function addSearchedBook(t,a,g,th) {
    await fetch(apiBase, {
        method:      'POST',
        credentials: 'include',
        headers:     { 'Content-Type': 'application/json' },
        body:        JSON.stringify({
            title: unescape(t),
                                    author: unescape(a),
                                    genre: unescape(g),
                                    status: 'Not Started',
                                    pagesRead: 0,
                                    totalPages: 0,
                                    notes: '',
                                    tags: '',
                                    goalEndDate: '',
                                    thumbnail: unescape(th),
                                    rating: 3
        })
    });
    document.getElementById('searchResults').innerHTML = '';
    await loadBooks();
    alert(`✅ Added "${unescape(t)}"!`);
}

// === Startup ===
window.addEventListener('DOMContentLoaded', () => {
    checkAuth();
});
