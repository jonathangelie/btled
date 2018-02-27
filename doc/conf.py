import sys
import os


sys.path.insert(0, os.path.abspath(".."))

html_theme = "sphinx_rtd_theme"
html_theme_path = ["_themes", ]

project = u'Bluetooth LE Daemon - btled'
copyright = u'Jonathan Gelie'
html_show_sphinx = False

# Add any Sphinx extension module names
extensions = [
    'sphinx.ext.autodoc',
    'sphinxcontrib.napoleon'
]

source_suffix = '.rst'
html_show_sourcelink = False
master_doc = 'index'
language = 'en'
pygments_style = 'default'
html_title = "btled"
html_static_path = ['_static']
htmlhelp_basename = 'btleddoc'
man_pages = [
    ('index', 'btleddoc', u'Bluetooth LE Documentation',
     [u'Jonathan Gelie'], 1)
]