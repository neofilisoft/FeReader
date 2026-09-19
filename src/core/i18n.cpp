#include "i18n.h"
#include <QHash>

namespace I18n {

// -------------------------------------------------------
// String tables - identical keys and values to original
// Python module.py LANG_STRINGS
// -------------------------------------------------------
static const QMap<QString, QString> s_en = {
    { "menu",           "File"                   },
    { "open",           "Open"                   },
    { "settings",       "Setting"                },
    { "convert",        "Convert"                },
    { "exit",           "Exit"                   },
    { "prev",           "Prev"                   },
    { "next",           "Next"                   },
    { "goto",           "Go to"                  },
    { "one_page",       "One Page"               },
    { "all_pages",      "All Pages"              },
    { "help",           "Help"                   },
    { "view_help",      "View Help"              },
    { "about",          "About FeReader"         },
    { "font",           "Font"                   },
    { "theme",          "Theme"                  },
    { "language",       "Language"               },
    { "theme_light",    "Light"                  },
    { "theme_dark",     "Dark"                   },
    { "language_en",    "English (US)"           },
    { "language_th",    "\u0e20\u0e32\u0e29\u0e32\u0e44\u0e17\u0e22" },
    { "settings_title", "Settings"               },
    { "convert_title",  "Convert"                },
    { "no_document",    "No document loaded."    },
    { "view",           "View"                   },
    { "vertical",       "Vertical"               },
    { "horizontal",     "Horizon"                },
};

static const QMap<QString, QString> s_th = {
    { "menu",           "\u0e44\u0e1f\u0e25\u0e4c"                                              },
    { "open",           "\u0e40\u0e1b\u0e34\u0e14"                                              },
    { "settings",       "\u0e15\u0e31\u0e49\u0e07\u0e04\u0e48\u0e32"                           },
    { "convert",        "\u0e41\u0e1b\u0e25\u0e07\u0e40\u0e2d\u0e01\u0e2a\u0e32\u0e23"        },
    { "exit",           "\u0e2d\u0e2d\u0e01"                                                    },
    { "prev",           "\u0e01\u0e48\u0e2d\u0e19\u0e2b\u0e19\u0e49\u0e32"                    },
    { "next",           "\u0e16\u0e31\u0e14\u0e44\u0e1b"                                        },
    { "goto",           "\u0e02\u0e49\u0e32\u0e21\u0e2b\u0e19\u0e49\u0e32"                    },
    { "one_page",       "\u0e17\u0e35\u0e25\u0e30\u0e2b\u0e19\u0e49\u0e32"                    },
    { "all_pages",      "\u0e17\u0e38\u0e01\u0e2b\u0e19\u0e49\u0e32"                          },
    { "help",           "\u0e0a\u0e48\u0e27\u0e22\u0e40\u0e2b\u0e25\u0e37\u0e2d"              },
    { "view_help",      "\u0e14\u0e39\u0e01\u0e32\u0e23\u0e0a\u0e48\u0e27\u0e22\u0e40\u0e2b\u0e25\u0e37\u0e2d" },
    { "about",          "\u0e40\u0e01\u0e35\u0e48\u0e22\u0e27\u0e01\u0e31\u0e1a FeReader"      },
    { "font",           "\u0e1f\u0e2d\u0e19\u0e15\u0e4c"                                       },
    { "theme",          "\u0e18\u0e35\u0e21"                                                    },
    { "language",       "\u0e20\u0e32\u0e29\u0e32"                                              },
    { "theme_light",    "\u0e42\u0e2b\u0e21\u0e14\u0e2a\u0e27\u0e48\u0e32\u0e07"               },
    { "theme_dark",     "\u0e42\u0e2b\u0e21\u0e14\u0e21\u0e37\u0e14"                           },
    { "language_en",    "English (US)"                                                          },
    { "language_th",    "\u0e20\u0e32\u0e29\u0e32\u0e44\u0e17\u0e22"                           },
    { "settings_title", "\u0e15\u0e31\u0e49\u0e07\u0e04\u0e48\u0e32"                           },
    { "convert_title",  "\u0e41\u0e1b\u0e25\u0e07\u0e40\u0e2d\u0e01\u0e2a\u0e32\u0e23"        },
    { "no_document",    "\u0e22\u0e31\u0e07\u0e44\u0e21\u0e48\u0e21\u0e35\u0e40\u0e2d\u0e01\u0e2a\u0e32\u0e23\u0e16\u0e39\u0e01\u0e40\u0e1b\u0e34\u0e14" },
    { "view",           "\u0e21\u0e38\u0e21\u0e21\u0e2d\u0e07"                                  },
    { "vertical",       "\u0e41\u0e19\u0e27\u0e15\u0e31\u0e49\u0e07"                           },
    { "horizontal",     "\u0e2d\u0e48\u0e32\u0e19\u0e41\u0e1a\u0e1a\u0e0b\u0e49\u0e32\u0e22\u0e02\u0e27\u0e32\u0e40\u0e2b\u0e21\u0e37\u0e2d\u0e19\u0e2b\u0e19\u0e31\u0e07\u0e2a\u0e37\u0e2d" },
};

// -------------------------------------------------------

QMap<QString, QString> stringsFor(const QString &lang)
{
    if (lang == "th")
        return s_th;
    return s_en;
}

QString tr(const QString &key, const QString &lang)
{
    const QMap<QString, QString> &m = (lang == "th") ? s_th : s_en;
    return m.value(key, key);
}

} // namespace I18n
