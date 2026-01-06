import sys
import os
import configparser
import json
import fitz 
from ebooklib import epub

APP_VERSION = "3.1.2"

if getattr(sys, "frozen", False):
    APP_DIR = os.path.dirname(sys.executable)
else:
    APP_DIR = os.path.dirname(os.path.abspath(__file__))

LANG_STRINGS = {
    "en": {
        "menu": "File", "open": "Open", "settings": "Setting", "convert": "Convert",
        "exit": "Exit", "prev": "Prev", "next": "Next", "goto": "Go to",
        "one_page": "One Page", "all_pages": "All Pages", "help": "Help",
        "view_help": "View Help", "about": "About FeReader", "font": "Font",
        "theme": "Theme", "language": "Language", "theme_light": "Light",
        "theme_dark": "Dark", "language_en": "English (US)", "language_th": "ภาษาไทย",
        "settings_title": "Settings", "convert_title": "Convert",
        "no_document": "No document loaded.", "view": "View",
        "vertical": "Vertical", "horizontal": "Horizon",
    },
    "th": {
        "menu": "ไฟล์", "open": "เปิด", "settings": "ตั้งค่า", "convert": "แปลงเอกสาร",
        "exit": "ออก", "prev": "ก่อนหน้า", "next": "ถัดไป", "goto": "ข้ามหน้า",
        "one_page": "ทีละหน้า", "all_pages": "ทุกหน้า", "help": "ช่วยเหลือ",
        "view_help": "ดูการช่วยเหลือ", "about": "เกี่ยวกับ FeReader", "font": "ฟอนต์",
        "theme": "ธีม", "language": "ภาษา", "theme_light": "โหมดสว่าง",
        "theme_dark": "โหมดมืด", "language_en": "English (US)", "language_th": "ภาษาไทย",
        "settings_title": "ตั้งค่า", "convert_title": "แปลงเอกสาร",
        "no_document": "ยังไม่มีเอกสารถูกเปิด", "view": "มุมมอง",
        "vertical": "แนวตั้ง", "horizontal": "อ่านแบบซ้ายขวาเหมือนหนังสือ",
    },
}

class ConfigManager:
    def __init__(self):
        self.config_path = os.path.join(APP_DIR, "settings.ini")
        self.config = configparser.ConfigParser()
        self._load_or_create_settings()

    def _load_or_create_settings(self):
        defaults = {
            "theme": "light", "font_family": "Segoe UI",
            "font_size": "16", "language": "en", "display_mode": "1"
        }
        if os.path.exists(self.config_path):
            try:
                self.config.read(self.config_path, encoding="utf-8")
            except Exception:
                self.config = configparser.ConfigParser()
        
        if "General" not in self.config:
            self.config["General"] = {}
        
        general = self.config["General"]
        for key, value in defaults.items():
            general.setdefault(key, value)
        self.save()

    def get(self, key, default=None):
        return self.config["General"].get(key, default)

    def set(self, key, value):
        self.config["General"][key] = str(value)

    def save(self):
        with open(self.config_path, "w", encoding="utf-8") as f:
            self.config.write(f)

class ConverterLogic:
    """Handles the actual file conversion logic separated from UI."""
    
    @staticmethod
    def text_to_pdf(input_path, output_path, password=None):
        with open(input_path, "r", encoding="utf-8", errors="ignore") as f:
            text = f.read()
        doc = fitz.open()
        page = doc.new_page()
        rect = fitz.Rect(50, 50, 550, 800)
        page.insert_textbox(rect, text, fontsize=11, align=0)
        ConverterLogic._save_doc(doc, output_path, password)

    @staticmethod
    def text_to_epub(input_path, output_path):
        with open(input_path, "r", encoding="utf-8", errors="ignore") as f:
            text = f.read()
        book = epub.EpubBook()
        book.set_identifier("fereader-convert")
        book.set_title(os.path.basename(input_path))
        book.set_language("en")
        chapter = epub.EpubHtml(title="Chapter 1", file_name="chap_1.xhtml", lang="en")
        chapter.content = f"<html><body><pre>{text}</pre></body></html>"
        book.add_item(chapter)
        book.toc = (chapter,)
        book.add_item(epub.EpubNcx())
        book.add_item(epub.EpubNav())
        book.spine = ["nav", chapter]
        epub.write_epub(output_path, book)

    @staticmethod
    def images_to_pdf(input_paths, output_path, password=None):
        doc = fitz.open()
        for img_path in input_paths:
            img_doc = fitz.open(img_path)
            rect = img_doc[0].rect
            page = doc.new_page(width=rect.width, height=rect.height)
            page.insert_image(rect, filename=img_path)
            img_doc.close()
        ConverterLogic._save_doc(doc, output_path, password)

    @staticmethod
    def _save_doc(doc, path, password):
        if password:
            doc.save(path, encryption=fitz.PDF_ENCRYPT_AES_128, owner_pw=password, user_pw=password)
        else:
            doc.save(path)
        doc.close()