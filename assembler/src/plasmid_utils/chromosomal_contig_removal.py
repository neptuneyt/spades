#from Bio.Blast import NCBIXML
import os
import sys
import ntpath
#import argparse
#from glob import glob
#from joblib import Parallel, delayed

# input - BLAST XML of scaffolds.fasta (?)
# output:
# if no alignment at all - nosig
# if good alignment (e<0.001):
#  if "plasmid" in name:
#    <3 matches and close length (0.9) - plasmids
#    others - plasmids_bad
# if "chrom"  in name - chrom
# others - unclass 



#chrom_hmms="/Nancy/mrayko/db/chromosomal_proteins_pfam/chromosome_uniq_hmms.hmm" ### Black list (2208)
#chrom_hmms="/Nancy/mrayko/db/chromosomal_proteins_pfam/chromosome_uniq_genbank_hmms.hmm" ### Deep Black list (508)
#chrom_hmms="/Nancy/mrayko/db/chromosomal_proteins_pfam/non_plasmid_pfamA_hmms.hmm" ### All non-plasmid Pfam (7317)
chrom_hmms="/Nancy/mrayko/db/chromosomal_proteins_pfam/2759_10fold_chrom_HMMs.hmm" ### ### 10 times more in chromosomes
hmmscan=" /Nancy/mrayko/Libs/hmmer-3.1b2-linux-intel-x86_64/binaries/hmmscan" 
prodigal="/Nancy/mrayko/Libs/Prodigal/prodigal"


name = os.path.splitext(ntpath.basename(sys.argv[1]))[0]
#print name

os.system (prodigal + " -p meta -i " + sys.argv[1] + " -a "+name+"_proteins.fa -o "+name+"_genes.fa" )
os.system (hmmscan + " -o "+name+"_out_pfam --tblout "+name+"_tblout --cpu 10 "+ chrom_hmms + " "+name+"_proteins.fa")


with open(name+"_tblout") as f:
    table = f.read().splitlines()

chrom_list=[]
for i in table[4:7]:
    if float (i.split()[4]) < 1e-06:
        chrom_list.append(i.split()[2].rsplit('_', 1)[0])

outfile=open (name+"_chromosomal_contigs_names.txt", "w")
for i in chrom_list: 
    outfile.write("%s\n" % i)

#os.system ("tail -n +4 "+name+"_tblout | head -n -10 | awk '$5<1e-06 {print $3}'| sed 's/_[^_]*$//g'| sort | uniq > "+name+"_chromosomal_contigs_names.txt")
