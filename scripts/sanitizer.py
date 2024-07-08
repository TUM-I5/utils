# SPDX-FileCopyrightText: 2024 Technical University of Munich
# 
# SPDX-License-Identifier: BSD-3-Clause

import re
import os
import pathlib
import subprocess
import datetime
import stat

from typing import List

class Mailmatch:
    SEISSOL_GROUP = 'Technical University of Munich'#'SeisSol Group'

    SUFFICES = {
        'in.tum.de': 'Technical University of Munich',
        'cit.tum.de': 'Technical University of Munich',
        'lrz.de': 'Leibniz Computing Centre',
        'tum.de': 'Technical University of Munich',
        'mytum.de': 'Technical University of Munich',
        'geophysik.uni-muenchen.de': 'Technical University of Munich',
        'intel.com': 'Intel Corporation'
    }

    EXPLICIT = {
        'lukas@krenz.land': 'Technical University of Munich',
        'ravil.mobile.com@gmail.com': 'Technical University of Munich',
        'g.brietzke@gmail.com': 'Leibniz Computing Centre'
    }

    # ignore online commits for now
    IGNORE_SUFFIX = ['users.noreply.github.com']

    @classmethod
    def matchMail(cls, address):
        [_,suffix] = address.split('@')
        if address in cls.EXPLICIT:
            return cls.EXPLICIT[address]
        elif suffix in cls.SUFFICES:
            return cls.SUFFICES[suffix]
        elif suffix in cls.IGNORE_SUFFIX:
            return None
        else:
            print(f'Unknown mail address/suffix: {address}')
            return None

class Settings:
    MAIN_DEVELOPER = 'Technical University of Munich'#'SeisSol Group'
    PROJECT_NAME = 'Utils'
    PROJECT_WEBPAGE = 'http://tum-i5.github.io/utils/' #'www.seissol.org'
    LICENSE = 'BSD-3-Clause'

    CPP_HEADER_ENDINGS = ['.h', '.hpp', '.cuh', '.hh']
    CPP_SOURCE_ENDINGS = ['.cpp', '.cu', '.cc']
    PY_SOURCE_ENDINGS = ['.py']
    SHELL_SOURCE_ENDINGS = ['.sh']
    CMAKE_SOURCE_ENDINGS = ['.cmake']
    CMAKE_FILE_NAMES = ['CMakeLists.txt']

class Span:
    def __init__(self, start, end=None):
        self.start = start
        if end is None:
            self.end = start
        else:
            self.end = end

    def unite(self, other):
        return Span(min(self.start, other.start), max(self.end, other.end))
    
    def extend(self, year):
        return Span(min(self.start, year), max(self.end, year))
    
    def __str__(self):
        if self.start == self.end:
            return f'{self.start}'
        else:
            return f'{self.start}-{self.end}'

def rootpath():
    path = subprocess.getoutput('git rev-parse --show-toplevel')
    return pathlib.Path(path)

def gitlogCopyrights(path, authorspans):
    authorlistAll = subprocess.getoutput(f'git log --pretty=format:"%ae;%ad" --date=format:%Y {path}')
    authorlist = authorlistAll.split('\n')

    for entry in authorlist:
        if entry.strip() != '':
            [mail,yearstr] = entry.split(';')
            author = Mailmatch.matchMail(mail)
            year = int(yearstr)
            if author is not None:
                if author not in authorspans:
                    authorspans[author] = Span(year)
                authorspans[author] = authorspans[author].extend(year)

def textCopyrights(copyrights, authorspans):
    crregex = re.compile(r'(?:(?:[Cc][Oo][Pp][Yy][Rr][Ii][Gg][Hh][Tt]\s+)|(?:SPDX\-FileCopyrightText\:\s*)|[\(\[]\s*[Cc]\s*[\)\]])+\s*(?:(\d+)(?:\s*-\s*(\d+))?)?[\s,;]*([\w\d\s\-_]*)')
    crlines = []
    for cr in copyrights:
        result = crregex.search(cr)
        holder = result.group(3).strip()
        year = result.group(1)
        year2 = result.group(2)
        if year2 is None:
            timespan = Span(int(year))
        else:
            timespan = Span(int(year), int(year2))
        
        if holder not in authorspans:
            authorspans[holder] = timespan
        else:
            authorspans[holder] = authorspans[holder].unite(timespan)

def licenseHeader(authorspans, commentstyle = '//'):
    # the main developer always comes first
    if Settings.MAIN_DEVELOPER is not None:
        authorlist = [Settings.MAIN_DEVELOPER] + [author for author in authorspans if author != Settings.MAIN_DEVELOPER]
    else:
        authorlist = [author for author in authorspans]

    # no titleline for now
    # titleline = f'{commentstyle} This file is part of {Settings.PROJECT_NAME} ( {Settings.PROJECT_WEBPAGE} ).'

    crlines = []
    for author in authorlist:
        # TODO: unite year spans
        timespan = authorspans[author]
        crlines += [f'{commentstyle} SPDX-FileCopyrightText: {timespan} {author}']
    return crlines + [f'{commentstyle} ', f'{commentstyle} SPDX-License-Identifier: {Settings.LICENSE}']

def makeLicense(path, copyrights, commentstyle = '//'):
    authorspans = {}
    gitlogCopyrights(path, authorspans)
    textCopyrights(copyrights, authorspans)

    # let the main developer always hold the copyright for the current year
    if Settings.MAIN_DEVELOPER is not None:
        currentYear = datetime.date.today().year
        if Settings.MAIN_DEVELOPER not in authorspans:
            authorspans[Settings.MAIN_DEVELOPER] = Span(currentYear)
        else:
            authorspans[Settings.MAIN_DEVELOPER] = authorspans[Settings.MAIN_DEVELOPER].extend(currentYear)

    return licenseHeader(authorspans, commentstyle)

def licenseCleaner(path, lines, commentstyle = '//'):
    inComment = False
    copyrights = []

    rest = []

    for i, line in enumerate(lines):
        if (line.strip().startswith('#') and commentstyle != '#') or (not inComment and not line.strip().startswith(commentstyle) and not line.strip().startswith('/*')):
            rest = lines[i:]
            break

        commentStart = line.find('/*')
        if commentStart != -1:
            inComment = True
        
        commentEnd = line.find('*/')
        if inComment or line.strip().startswith(commentstyle):
            copyrightpos = line.find('Copyright')
            if copyrightpos != -1 and (commentEnd == -1 or copyrightpos < commentEnd) and (commentStart == -1 or copyrightpos > commentStart):
                copyrights += [line]
        
        if commentEnd != -1:
            inComment = False
    
    return makeLicense(path, copyrights, commentstyle) + rest

def lastNewline(lines):
    if lines[-1] != '':
        return lines + ['']
    else:
        return lines

def cppCanonicalHeaders(path: pathlib.Path, lines: List[str]):
    # remove pragma once
    for i, line in enumerate(lines):
        if line.strip() == '#pragma once':
            preheader = lines[:i]
            innerlines = lines[i+1:]
            break
    
    # TODO: check if there's an include guard at all
    # remove default include guard
    ifndefline = None
    innerlinesPre = None
    for i, line in enumerate(lines):
        if line.strip().startswith('#ifndef'):
            ifndefline = i
        elif line.strip().startswith('#define') and ifndefline is not None:
            preheader = lines[:ifndefline]
            innerlinesPre = lines[i+1:]
            break
        elif not (line.strip().startswith('//') or line.strip() == ''):
            preheader = lines[:i]
            innerlinesPre = lines[i+1:]
            break
    innerlines = None
    for i, line in reversed(list(enumerate(innerlinesPre))):
        if line.strip().startswith('#endif'):
            innerlines = innerlinesPre[:i]
            break
        elif not (line.strip().startswith('//') or line.strip() == ''):
            innerlines = innerlinesPre[:(i+1)]
            break

    # re-insert new include guard
    strpath = str(path.relative_to(rootpath()))
    sanitizedpath = strpath.replace('/', '_').replace('.', '_').upper()
    projectprefix = Settings.PROJECT_NAME.upper()
    guard = f'{projectprefix}_{sanitizedpath}_'

    return preheader + [f'#ifndef {guard}', f'#define {guard}'] + innerlines + [f'#endif // {guard}', '']

def addShebang(path, lines, program):
    # only add shebang for files that are executable

    stats = path.stat().st_mode
    if (stats & stat.S_IXUSR) != 0 or (stats & stat.S_IXGRP) != 0 or (stats & stat.S_IXOTH) != 0:
        shebang = f'#!/usr/bin/env {program}'
        if lines[0].strip().startswith('#!'):
            return [shebang, ''] + lines[1:]
        else:
            return [shebang, ''] + lines
    else:
        return lines

def sanitizeLineEndings(lines):
    return [line if line.endswith('\n') else line + '\n' for line in lines]

def processFile(path, dryrun):
    pathobj = pathlib.Path(path).resolve()
    with open(path) as file:
        lines = file.readlines()

    if pathobj.suffix in Settings.PY_SOURCE_ENDINGS:
        lines = licenseCleaner(pathobj, lines, '#')
        lines = addShebang(pathobj, lines, 'python3')
    if pathobj.suffix in Settings.SHELL_SOURCE_ENDINGS:
        lines = licenseCleaner(pathobj, lines, '#')
        lines = addShebang(pathobj, lines, 'sh')
    if pathobj.suffix in Settings.CMAKE_SOURCE_ENDINGS or pathobj.name in Settings.CMAKE_FILE_NAMES:
        lines = licenseCleaner(pathobj, lines, '#')
    if pathobj.suffix in Settings.CPP_HEADER_ENDINGS + Settings.CPP_SOURCE_ENDINGS:
        lines = licenseCleaner(pathobj, lines, '//')
    if pathobj.suffix in Settings.CPP_HEADER_ENDINGS:
        lines = cppCanonicalHeaders(pathobj, lines)
    lines = lastNewline(lines)
    lines = sanitizeLineEndings(lines)

    if not dryrun:
        with open(path, 'w') as file:
            file.writelines(lines)

def main():
    print('SeisSol File Sanitizer')
    # dirs = [('generated_code', '*{}'), ('src', '**/*{}'), ('src', '**/CMakeLists.txt'), ('auto_tuning', '**/*{}')]
    dirs = [('.', '*{}'), ('tests', '**/*{}')]
    for ending in Settings.CPP_HEADER_ENDINGS + Settings.CPP_SOURCE_ENDINGS + Settings.PY_SOURCE_ENDINGS + Settings.SHELL_SOURCE_ENDINGS + Settings.CMAKE_SOURCE_ENDINGS:
        for directory, globstr in dirs:
            for file in (rootpath() / directory).glob(globstr.format(ending)):
                print(file)
                processFile(file, False)
    print(rootpath() / 'CMakeLists.txt')
    processFile(rootpath() / 'CMakeLists.txt', False)
    print('Done.')

if __name__ == '__main__':
    main()
