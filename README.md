# 📚 BookTracker

A full‑stack book–tracking web app with:

- **User accounts** (cookie‑based sessions)  
- **CRUD** operations on your personal book collection  
- **Reading progress** (pages read vs total pages)  
- **Reading sessions** & **progress chart** (via Chart.js)  
- **Pace estimator** & **reading streak**  
- **Search** & **add** books from the Google Books API  
- **Dark mode** & **collapsible** “Add a Book” panel  
- **Filters**, **sorting**, and **inline rating**

---

## 🚀 Features

- **Authentication**: Sign up, log in, and maintain sessions with secure HTTP‑only cookies.  
- **Book CRUD**: Add, edit status/notes/tags/goal date/progress/rating, or delete books.  
- **Progress Tracking**:  
  - Slider and numeric input for pages‑read vs total pages  
  - Automatically computes “% complete”  
- **Reading Sessions**:  
  - Start/stop a session to log pages read over time  
  - Displays a line chart of pages‑read per day  
  - Calculates average pages/day and consecutive‑day reading streak  
- **Google Books Integration**: Search titles/authors, preview covers, and add to your collection.  
- **Responsive UI**: TailwindCSS for mobile‑friendly layouts and dark‑mode support.  

---

## 🛠 Tech Stack

- **Backend**: C++17, [SQLite](https://sqlite.org/), [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp), [nlohmann/json](https://github.com/nlohmann/json), [cpp-httplib](https://github.com/yhirose/cpp-httplib)  
- **Frontend**: Vanilla JavaScript, TailwindCSS (via CDN), Chart.js (via CDN)  
- **API**: Google Books API  

---

## 📋 Prerequisites

- C++17‑compatible compiler (gcc, clang, MSVC)  
- [CMake](https://cmake.org/)  
- SQLite3  
- A GitHub repository (optional, for version control)  

---

## ⚙️ Installation & Build

1. **Clone the repo**  
   ```bash
   git clone https://github.com/YOUR_USERNAME/BookTracker.git
   cd BookTracker
