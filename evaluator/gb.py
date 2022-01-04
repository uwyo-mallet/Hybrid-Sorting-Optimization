#!/usr/bin/env python3
"""
Google benchmark result comparer/viewer.

"""

from functools import partial

import dash

from .layout import gen_layout
from .loader import CLOCKS, DATA_TYPES

# Debug Options
# pd.set_option("display.max_columns", None)
# pd.set_option("display.max_rows", 500)
# pd.set_option("display.max_columns", 500)
# pd.set_option("display.width", 1000)

app = dash.Dash(__name__)
app.layout = partial(gen_layout, CLOCKS, DATA_TYPES, None)
