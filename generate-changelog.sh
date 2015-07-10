#!/bin/bash
#
# Generate markdown ChangeLog files based on git history.

outfile=$1

# Gracefully do nothing if the file exists
if [ -e ${outfile} ]; then
	exit 0;
fi

tmp=$(mktemp)
touch ${tmp}

prev_tag=""
echo "# ChangeLog"
for tag in $(git tag -l [0-9]* | sort -V); do
	# Prepend tags to the output because we want the ChangeLog in the
	# reverse order, e.g. 2.0.0, 1.0.1 and 1.0.0.
	{
		date=$(git show --date=short --format='tformat:%cd' -s ${tag})
		echo -e "\n## ${tag} - ${date}\n"
		git --no-pager log --no-merges --format='format: - %s%n' \
		    ${prev_tag:+${prev_tag}..}${tag} | cat - ${tmp}
	} > ${tmp}.1
	mv ${tmp}.1 ${tmp}
	prev_tag=${tag}
done

# Move the file to the final destination
mv ${tmp} ${outfile}
