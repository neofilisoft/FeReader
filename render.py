import os
import shutil
import tempfile
import posixpath
import fitz 
import ebooklib
from ebooklib import epub
from bs4 import BeautifulSoup
from PySide6.QtGui import QImage, QPixmap, QPainter
from PySide6.QtCore import Qt, QUrl

class RenderEngine:
    def __init__(self):
        self.pdf_doc = None
        self.epub_temp_dir = None
        self.pages = []  
        self.book_type = None

    def cleanup(self):
        """Clean up temp files and close documents."""
        if self.epub_temp_dir and os.path.isdir(self.epub_temp_dir):
            try:
                shutil.rmtree(self.epub_temp_dir, ignore_errors=True)
            except Exception:
                pass
        self.epub_temp_dir = None
        if self.pdf_doc:
            self.pdf_doc.close()
            self.pdf_doc = None

    def load_pdf(self, path, password_callback=None):
        self.cleanup()
        self.book_type = "pdf"
        self.pdf_doc = fitz.open(path)
        
        if getattr(self.pdf_doc, "needs_pass", False):
            if password_callback:
                pw = password_callback()
                if not pw or not self.pdf_doc.authenticate(pw):
                    self.pdf_doc.close()
                    raise ValueError("Password required or incorrect")
            else:
                self.pdf_doc.close()
                raise ValueError("Password required")

        self.pages = list(range(self.pdf_doc.page_count))
        return len(self.pages)

    def load_epub(self, path):
        self.cleanup()
        self.book_type = "epub"
        self.epub_temp_dir = tempfile.mkdtemp(prefix="fereader_epub_")
        
        book = epub.read_epub(path)
        
        for item in book.get_items():
            content = item.get_content()
            rel_path = item.file_name.replace("/", os.sep)
            out_path = os.path.join(self.epub_temp_dir, rel_path)
            os.makedirs(os.path.dirname(out_path), exist_ok=True)
            with open(out_path, "wb") as f:
                f.write(content)

        # Process HTML
        self.pages = []
        for item in book.get_items_of_type(ebooklib.ITEM_DOCUMENT):
            html_bytes = item.get_content()
            html = html_bytes.decode("utf-8", errors="ignore")
            html_dir = posixpath.dirname(item.file_name)
            soup = BeautifulSoup(html, "html.parser")

            for img_tag in soup.find_all("img"):
                src = img_tag.get("src")
                if src:
                    rel = posixpath.normpath(posixpath.join(html_dir, src))
                    local_path = os.path.join(self.epub_temp_dir, rel.replace("/", os.sep))
                    file_url = QUrl.fromLocalFile(local_path).toString()
                    img_tag["src"] = file_url
            
            self.pages.append(str(soup))
        
        if not self.pages:
            self.pages.append("<h3>No readable content found.</h3>")
        
        return self.pages

    def get_pdf_page_pixmap(self, index, zoom=1.0):
        if not self.pdf_doc or not (0 <= index < self.pdf_doc.page_count):
            return None
        try:
            page = self.pdf_doc.load_page(index)
            if zoom < 0.1: zoom = 0.1
            mat = fitz.Matrix(zoom, zoom)
            pix = page.get_pixmap(matrix=mat, alpha=True)
            img = QImage(pix.samples, pix.width, pix.height, pix.stride, QImage.Format_RGBA8888)
            return QPixmap.fromImage(img.copy())
        except Exception as e:
            print(f"Render Error: {e}")
            return None

    def get_pdf_spread_pixmap(self, left_index, zoom=1.0):
        """Render two pages side-by-side."""
        left_pix = self.get_pdf_page_pixmap(left_index, zoom)
        if left_pix is None:
            return None
        
        right_pix = None
        if left_index + 1 < self.pdf_doc.page_count:
            right_pix = self.get_pdf_page_pixmap(left_index + 1, zoom)
        
        if right_pix is None:
            return left_pix

        target_height = max(left_pix.height(), right_pix.height())
        left_scaled = left_pix.scaledToHeight(target_height, Qt.SmoothTransformation)
        right_scaled = right_pix.scaledToHeight(target_height, Qt.SmoothTransformation)

        spread = QPixmap(left_scaled.width() + right_scaled.width(), target_height)
        spread.fill(Qt.transparent)
        painter = QPainter(spread)
        painter.drawPixmap(0, 0, left_scaled)
        painter.drawPixmap(left_scaled.width(), 0, right_scaled)
        painter.end()
        return spread
    
    def get_initial_zoom(self, view_width, view_height):
        if self.pdf_doc and self.pdf_doc.page_count > 0:
            page = self.pdf_doc.load_page(0)
            z_h = view_height / page.rect.height
            z_w = view_width / page.rect.width
            return round(min(z_h, z_w), 2)
        return 1.0