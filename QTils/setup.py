from distutils.core import setup, Extension
from numpy import get_include as numpy_get_include

try:
	from distutils.command.build_py import build_py_2to3 as build_py
except ImportError:
	from distutils.command.build_py import build_py

def doall():
	setup(name='_QTils', version='1.0',
		 description = 'a package providing access to RJVBs QTils library',
		 ext_modules = [Extension('_QTils',
							 sources=['QTils_wrap.c'],
							 depends=['QTilities.h'],
							 extra_compile_args=['-g','-framework','QTils'],
							 extra_link_args=['-g','-framework','QTils']
							 )])

	setup(name='QTils', version='1.0',
		 description = 'a package providing access to RJVBs QTils library',
		 py_modules=['QTils'], cmdclass={'build_py':build_py}, )

doall()

import os
os.system("rm -rf build/lib")
