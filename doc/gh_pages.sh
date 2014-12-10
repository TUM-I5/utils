#!/bin/bash

# @file
#  This file is part of UTILS
#
# UTILS is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# UTILS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with UTILS.  If not, see <http://www.gnu.org/licenses/>.
#
# @copyright 2014 Technische Universitaet Muenchen
# @author Sebastian Rettenberger <rettenbs@in.tum.de>

export REPO="$(pwd | sed s,^/home/travis/build/,,g)"
echo -e "Current Repo:$REPO --- Travis Branch:$TRAVIS_BRANCH" 

git config --global user.email "rettenbs@in.tum.de"
git config --global user.name "Travis" 

if [ "$TRAVIS_BRANCH" == "master" ]; then
	doxygen
	
	git clone --quiet --branch=gh-pages https://${GH_TOKEN}@github.com/TUM-I5/utils.git gh-pages > /dev/null
	cd gh-pages
	
	git rm -rf .
	cp -r ../html/* .
	git add -f .
	git commit -m "Travis build $TRAVIS_BUILD_NUMBER pushed to gh-pages"
	git push -fq origin gh-pages > /dev/null
fi 