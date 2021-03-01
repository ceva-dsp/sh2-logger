#!/usr/bin/bash
if [ $# -lt 2 ]
then
    cat <<EOF 1>&2
Usage: $0 <input> <output_prefix>

Split an input .dsf file into one .csv file per data channel. This
removes channel identifiers and flattens the channel definition line.

Note that this *only* translates header and data (and doesn't record
file-wide properties such as calibration records or other data such as
period declaration events).

EOF
    exit 1
fi
f=$1
prefix=$2
mkdir -p $(dirname $prefix)
for channel in $(grep '^\+' $f | tr -d '+' | cut -d ' ' -f 1)
do
    name=$(grep "^!${channel} name" $f | head -1 | cut -d '"' -f 2)
    if [ "${name}" == "" ]
    then
        name=${channel}
    fi
    outFN="${prefix}.${channel}.${name}.csv"
    header=$(grep "^\+${channel} " $f)
    echo ${header} | cut -d ' ' -f 1 --complement | awk -v FS=, '
{
    for (i = 1; i<=NF; i++){
        split($i, a, "[")
        if (length(a) == 1){
            printf "%s", a[1] 
        }else{
            split(a[2], aa, "]")
            for (k=1; k<= length(aa[1]);k++){
                printf "%s", a[1] "." substr(aa[1], k, 1)  aa[2]
                if (k != length(aa[1])){
                    printf FS
                }
            }
        }
        if (i != NF){
            printf FS
        }
    }
    printf ORS
}' > ${outFN}
    grep "^\.${channel} " $f | cut -d ' ' -f 1 --complement >> ${outFN}
done
