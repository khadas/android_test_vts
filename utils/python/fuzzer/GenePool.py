

def CreateGenePool(count, generator, fuzzer, **kwargs):
  genes = []
  for index in range(count):
    gene = generator(**kwargs)
    genes.append(fuzzer(gene))
  return genes


def Evolve(genes, fuzzer):
  new_genes = []
  for gene in genes:
    # TODO: consider cross over
    new_genes.append(fuzzer(gene))
  return new_genes
