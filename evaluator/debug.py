"""
Debug Module.

Sets helpful pandas configuration to make debugging easier.
Only import this if you need to debug something.
"""
from pprint import pprint

import pandas as pd

print("Debug Module Enabled")

# Debug Options
pd.set_option("display.max_columns", None)
pd.set_option("display.max_rows", 500)
pd.set_option("display.max_columns", 500)
pd.set_option("display.width", 1000)
