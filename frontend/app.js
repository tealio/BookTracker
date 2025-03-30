const api = 'http://localhost:8080/api/books';

// 📊 Render Stats Dashboard
function renderStats(books) {
    const statsDiv = document.getElementById('stats');
    const total = books.length;
    const completed = books.filter(b => b.status === 'Completed').length;
    const reading = books.filter(b => b.status === 'Reading').length;
    const notStarted = books.filter(b => b.status === 'Not Started').length;

    const ratings = books.filter(b => b.rating).map(b => b.rating);
    const avgRating = ratings.length
    ? (ratings.reduce((a, b) => a + b, 0) / ratings.length).toFixed(1)
    : '—';

    const genreCounts = {};
    books.forEach(b => {
        if (b.genre) {
            genreCounts[b.genre] = (genreCounts[b.genre] || 0) + 1;
        }
    });
    const topGenre = Object.entries(genreCounts).sort((a, b) => b[1] - a[1])[0]?.[0] || '—';

    statsDiv.innerHTML = `
    <div><strong>Total Books:</strong> ${total}</div>
    <div><strong>Completed:</strong> ${completed}</div>
    <div><strong>Reading:</strong> ${reading}</div>
    <div><strong>Not Started:</strong> ${notStarted}</div>
    <div><strong>Average Rating:</strong> ${avgRating} ⭐️</div>
    <div><strong>Top Genre:</strong> ${topGenre}</div>
    `;
}

// 📥 Load and render books
async function loadBooks() {
    const res = await fetch(api);
    let books = await res.json();

    renderStats(books);

    const container = document.getElementById('books');
    container.innerHTML = '';

    const filterValue = document.getElementById('filter')?.value || 'all';
    books = books.filter(book => {
        if (filterValue === 'reading') return book.status === "Reading";
        if (filterValue === 'completed') return book.status === "Completed";
        if (filterValue === 'not-started') return book.status === "Not Started";
        return true;
    });

    const sortValue = document.getElementById('sort')?.value || 'title-asc';
    books.sort((a, b) => {
        if (sortValue === 'title-asc') return a.title.localeCompare(b.title);
        if (sortValue === 'title-desc') return b.title.localeCompare(a.title);
        if (sortValue === 'progress-asc') return a.progress - b.progress;
        if (sortValue === 'progress-desc') return b.progress - a.progress;
        if (sortValue === 'rating-asc') return a.rating - b.rating;
        if (sortValue === 'rating-desc') return b.rating - a.rating;
        return 0;
    });

    books.forEach(book => {
        const div = document.createElement('div');
        div.className = "flex gap-4 bg-white dark:bg-gray-800 p-4 rounded shadow hover:shadow-lg transition";

        const imgHTML = book.thumbnail
        ? `<img src="${book.thumbnail}" alt="Cover" class="h-24 rounded" />`
        : '';

        const progressHTML = book.status === "Reading" ? `
        <label class="block mt-2 text-gray-800 dark:text-gray-300">Progress: <span id="progressDisplay-${book.id}">${book.progress}</span>%</label>
        <input type="range" min="0" max="100" value="${book.progress}"
        oninput="document.getElementById('progressDisplay-${book.id}').textContent = this.value"
        onchange="updateBook(${book.id})"
        class="w-full">` : '';

        const emojiHTML = [1, 2, 3, 4, 5].map(rating => {
            const emoji = ["😡", "😕", "😐", "😊", "🤩"][rating - 1];
            const isSelected = book.rating === rating;
            return `<button onclick="setRating(${book.id}, ${rating})" class="${isSelected ? 'scale-125' : 'opacity-50'} hover:scale-110 transition">${emoji}</button>`;
        }).join('');

        div.innerHTML = `
        ${imgHTML}
        <div class="flex-1">
        <div class="flex justify-between items-start">
        <h3 class="text-lg font-bold text-gray-900 dark:text-white">${book.title}</h3>
        <button class="text-red-500" onclick="deleteBook(${book.id})">✕</button>
        </div>
        <p class="text-gray-800 dark:text-gray-300"><strong>Author:</strong> ${book.author}</p>
        <p class="text-gray-800 dark:text-gray-300"><strong>Genre:</strong> ${book.genre || 'N/A'}</p>

        <label class="block mt-2 text-gray-800 dark:text-gray-300">Status:
        <select id="status-${book.id}" onchange="updateBook(${book.id})"
        class="w-full mt-1 p-2 border rounded bg-white text-gray-900 dark:bg-gray-700 dark:text-white dark:border-gray-600">
        <option value="Not Started" ${book.status === "Not Started" ? "selected" : ""}>Not Started</option>
        <option value="Reading" ${book.status === "Reading" ? "selected" : ""}>Reading</option>
        <option value="Completed" ${book.status === "Completed" ? "selected" : ""}>Completed</option>
        </select>
        </label>

        ${progressHTML}

        <label class="block mt-2 text-gray-800 dark:text-gray-300">Notes:
        <textarea id="notes-${book.id}" onchange="updateBook(${book.id})"
        class="w-full mt-1 p-2 border rounded bg-white text-gray-900 dark:bg-gray-700 dark:text-white dark:border-gray-600"
        rows="3">${book.notes || ""}</textarea>
        </label>

        ${book.status === "Completed" ? `
            <label class="block mt-4 text-gray-800 dark:text-gray-300">Rating:</label>
            <div class="flex gap-2 mt-1 text-2xl">${emojiHTML}</div>
            ` : ''}
            </div>
            `;

            container.appendChild(div);
    });
}

async function addBook(e) {
    e.preventDefault();
    const title = document.getElementById('title').value;
    const author = document.getElementById('author').value;
    const genre = document.getElementById('genre').value;

    await fetch(api, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            title,
            author,
            genre,
            status: "Not Started",
            progress: 0,
            notes: "",
            thumbnail: ""
        })
    });

    document.getElementById('addBookForm').reset();
    await loadBooks();
}

async function deleteBook(id) {
    await fetch(`${api}/${id}`, { method: 'DELETE' });
    await loadBooks();
}

async function updateBook(id) {
    const status = document.getElementById(`status-${id}`)?.value || "Not Started";
    const notes = document.getElementById(`notes-${id}`)?.value || "";
    const progress = status === "Reading"
    ? parseInt(document.getElementById(`progressDisplay-${id}`)?.textContent || "0")
    : 0;

    await fetch(`${api}/${id}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ status, progress, notes })
    });

    await loadBooks();
}

async function setRating(id, rating) {
    const status = document.getElementById(`status-${id}`)?.value || "Not Started";
    const notes = document.getElementById(`notes-${id}`)?.value || "";
    const progress = status === "Reading"
    ? parseInt(document.getElementById(`progressDisplay-${id}`)?.textContent || "0")
    : 0;

    await fetch(`${api}/${id}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ status, progress, notes, rating })
    });

    await loadBooks();
}

// 🌙 Dark Mode Toggle
function toggleDarkMode() {
    document.documentElement.classList.toggle('dark');
    localStorage.setItem('darkMode', document.documentElement.classList.contains('dark') ? 'on' : 'off');
}

window.addEventListener('DOMContentLoaded', () => {
    if (localStorage.getItem('darkMode') === 'on') {
        document.documentElement.classList.add('dark');
    }

    loadBooks();
});

document.getElementById('filter').addEventListener('change', loadBooks);
document.getElementById('sort').addEventListener('change', loadBooks);

// 🔍 Google Books Search
document.getElementById('searchForm').addEventListener('submit', searchBooks);

async function searchBooks(e) {
    e.preventDefault();
    const query = document.getElementById('searchInput').value.trim();
    if (!query) return;

    const res = await fetch(`http://localhost:8080/api/search/${encodeURIComponent(query)}`);
    const data = await res.json();

    const resultsDiv = document.getElementById('searchResults');
    resultsDiv.innerHTML = '<h3 class="text-lg font-semibold mb-2 text-gray-900 dark:text-white">Search Results:</h3>';

    data.items?.slice(0, 5).forEach(item => {
        const info = item.volumeInfo;
        const title = info.title || "Untitled";
        const author = (info.authors && info.authors[0]) || "Unknown";
        const genre = (info.categories && info.categories[0]) || "";
        const thumbnail = (info.imageLinks && info.imageLinks.thumbnail) || "";

        const div = document.createElement('div');
        div.className = 'flex gap-4 bg-gray-100 dark:bg-gray-700 p-4 rounded shadow hover:shadow-md transition';
        div.innerHTML = `
        ${thumbnail ? `<img src="${thumbnail}" alt="Cover" class="h-24 rounded" />` : ""}
        <div class="flex-1">
        <strong class="block text-lg text-gray-900 dark:text-white">${title}</strong>
        <p class="mb-2 text-sm text-gray-700 dark:text-gray-300">${author}</p>
        <button class="bg-primary text-white px-3 py-1 rounded hover:bg-indigo-600 dark:bg-indigo-500 dark:hover:bg-indigo-400 transition"
        onclick='addSearchedBook("${escape(title)}", "${escape(author)}", "${escape(genre)}", "${escape(thumbnail)}")'>➕ Add</button>
        </div>
        `;
        resultsDiv.appendChild(div);
    });
}

async function addSearchedBook(title, author, genre, thumbnail) {
    await fetch(api, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            title: unescape(title),
                             author: unescape(author),
                             genre: unescape(genre),
                             status: "Not Started",
                             progress: 0,
                             notes: "",
                             thumbnail: unescape(thumbnail)
        })
    });

    await loadBooks();
    alert(`✅ Added "${unescape(title)}" to your collection!`);
}
