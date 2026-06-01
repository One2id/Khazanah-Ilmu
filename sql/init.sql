-- Khazanah Ilmu — Database Initialisation
-- Run automatically by Docker on first container start

USE khazanah_ilmu;

-- ─────────────────────────────────────────────
-- TABLE CREATION (dependency order)
-- ─────────────────────────────────────────────

CREATE TABLE IF NOT EXISTS language (
    language_id   INT AUTO_INCREMENT PRIMARY KEY,
    language_code VARCHAR(10)  NOT NULL UNIQUE,
    language_name VARCHAR(100) NOT NULL,
    script_type   VARCHAR(50),
    world_region  VARCHAR(100)
);

CREATE TABLE IF NOT EXISTS author (
    author_id    INT AUTO_INCREMENT PRIMARY KEY,
    author_name  VARCHAR(200) NOT NULL,
    nationality  VARCHAR(100),
    birth_year   INT,
    death_year   INT,
    era          VARCHAR(100)
);

CREATE TABLE IF NOT EXISTS book (
    book_id           INT AUTO_INCREMENT PRIMARY KEY,
    title             VARCHAR(300) NOT NULL,
    author_id         INT,
    language_id       INT,
    genre             VARCHAR(100),
    origin_country    VARCHAR(100),
    published_year    INT,
    historical_era    VARCHAR(100),
    copies_available  INT DEFAULT 1,
    FOREIGN KEY (author_id)   REFERENCES author(author_id)   ON DELETE SET NULL,
    FOREIGN KEY (language_id) REFERENCES language(language_id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS member (
    member_id       INT AUTO_INCREMENT PRIMARY KEY,
    full_name       VARCHAR(200) NOT NULL,
    email           VARCHAR(200) UNIQUE,
    phone           VARCHAR(20),
    membership_date DATE DEFAULT (CURRENT_DATE),
    status          ENUM('active', 'suspended') DEFAULT 'active'
);

CREATE TABLE IF NOT EXISTS loan (
    loan_id      INT AUTO_INCREMENT PRIMARY KEY,
    member_id    INT NOT NULL,
    book_id      INT NOT NULL,
    loan_date    DATE DEFAULT (CURRENT_DATE),
    due_date     DATE NOT NULL,
    return_date  DATE DEFAULT NULL,
    status       ENUM('active', 'returned', 'overdue') DEFAULT 'active',
    FOREIGN KEY (member_id) REFERENCES member(member_id) ON DELETE CASCADE,
    FOREIGN KEY (book_id)   REFERENCES book(book_id)     ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS fine (
    fine_id     INT AUTO_INCREMENT PRIMARY KEY,
    loan_id     INT NOT NULL UNIQUE,
    amount      DECIMAL(8,2) NOT NULL DEFAULT 0.00,
    paid_status ENUM('unpaid', 'paid') DEFAULT 'unpaid',
    fine_date   DATE DEFAULT (CURRENT_DATE),
    FOREIGN KEY (loan_id) REFERENCES loan(loan_id) ON DELETE CASCADE
);

-- ─────────────────────────────────────────────
-- SAMPLE DATA
-- ─────────────────────────────────────────────

-- Languages
INSERT INTO language (language_code, language_name, script_type, world_region) VALUES
('AR', 'Arabic',   'Arabic', 'Middle East'),
('MS', 'Malay',    'Latin',  'Southeast Asia'),
('ZH', 'Chinese',  'CJK',    'East Asia'),
('FA', 'Persian',  'Arabic', 'Middle East'),
('LA', 'Latin',    'Latin',  'Europe'),
('IT', 'Italian',  'Latin',  'Europe');

-- Authors
INSERT INTO author (author_name, nationality, birth_year, death_year, era) VALUES
('Ibn Khaldun',          'Tunisian',    1332,  1406, 'Medieval Islamic'),
('Al-Biruni',            'Khwarazmian', 973,   1048, 'Ghaznavid Era'),
('Tun Sri Lanang',       'Malay',       1565,  1659, 'Melaka Sultanate'),
('Sun Tzu',              'Chinese',     -544,  -496, 'Spring and Autumn Period'),
('Confucius',            'Chinese',     -551,  -479, 'Spring and Autumn Period'),
('Jalal al-Din Rumi',    'Persian',     1207,  1273, 'Seljuk Era'),
('Ferdowsi',             'Persian',     940,   1020, 'Samanid Era'),
('Dante Alighieri',      'Italian',     1265,  1321, 'Medieval European'),
('Niccolo Machiavelli',  'Italian',     1469,  1527, 'Renaissance'),
('Anonymous Malay',      'Malay',       NULL,  NULL, 'Melaka Sultanate');

-- Books
INSERT INTO book (title, author_id, language_id, genre, origin_country, published_year, historical_era, copies_available) VALUES
('Muqaddimah',           1,  1, 'History',            'Tunisia',  1377, 'Medieval Islamic',   3),
('Kitab al-Hind',        2,  1, 'Geography',          'Persia',   1030, 'Medieval Islamic',   2),
('Sejarah Melayu',       3,  2, 'History',            'Melaka',   1612, 'Early Modern',       2),
('Hikayat Hang Tuah',    10, 2, 'Literature',         'Melaka',   1700, 'Early Modern',       3),
('The Art of War',       4,  3, 'Military Strategy',  'China',    -500, 'Classical',          4),
('Analects',             5,  3, 'Philosophy',         'China',    -479, 'Classical',          3),
('Masnavi',              6,  4, 'Sufi Poetry',        'Persia',   1258, 'Medieval Islamic',   2),
('Shahnameh',            7,  4, 'Epic Poetry',        'Persia',   1010, 'Medieval Islamic',   2),
('Divine Comedy',        8,  6, 'Literature',         'Italy',    1320, 'Medieval European',  3),
('The Prince',           9,  6, 'Political Philosophy','Italy',   1532, 'Renaissance',        2);

-- Members
INSERT INTO member (full_name, email, phone, membership_date, status) VALUES
('Ahmad bin Abdullah',       'ahmad@email.com',    '0123456789', '2024-01-15', 'active'),
('Siti Aishah bte Ismail',   'siti@email.com',     '0187654321', '2024-02-20', 'active'),
('Chen Wei Ming',            'chen@email.com',     '0165432198', '2024-03-10', 'active'),
('Rajesh Kumar',             'rajesh@email.com',   '0112345678', '2024-04-05', 'active'),
('Fatimah bte Hassan',       'fatimah@email.com',  '0198765432', '2024-05-12', 'active'),
('Yusuf al-Rashid',          'yusuf@email.com',    '0134567890', '2024-01-01', 'suspended');

-- Loans
INSERT INTO loan (member_id, book_id, loan_date, due_date, return_date, status) VALUES
(1, 1, '2025-10-01', '2025-10-15', '2025-10-12', 'returned'),
(2, 3, '2025-10-05', '2025-10-19', NULL,          'overdue'),
(3, 5, '2025-09-01', '2025-09-15', NULL,          'overdue'),
(4, 7, '2025-10-10', '2025-10-24', '2025-10-20',  'returned'),
(5, 9, '2025-09-15', '2025-09-29', NULL,          'overdue'),
(1, 2, '2025-10-20', '2025-11-03', NULL,          'active'),
(6, 6, '2025-08-01', '2025-08-15', NULL,          'overdue'),
(3, 8, '2025-08-20', '2025-09-03', '2025-09-10',  'returned');

-- Fines (for overdue or late-returned loans)
INSERT INTO fine (loan_id, amount, paid_status, fine_date) VALUES
(2,  6.00,  'unpaid', '2025-10-20'),
(3,  5.50,  'unpaid', '2025-09-16'),
(5,  8.00,  'unpaid', '2025-09-30'),
(7,  12.00, 'unpaid', '2025-08-16'),
(8,  3.50,  'paid',   '2025-09-11');
