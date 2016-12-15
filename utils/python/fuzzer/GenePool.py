#!/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

def CreateGenePool(count, generator, fuzzer, **kwargs):
  """Creates a gene pool, a set of test input data.

  Args:
    count: integer, the size of the pool.
    generator: function pointer, which can generate the data.
    fuzzer: function pointer, which can mutate the data.
    **kwargs: the args to the generator function pointer.

  Returns:
    a list of generated data.
  """
  genes = []
  for index in range(count):
    gene = generator(**kwargs)
    genes.append(fuzzer(gene))
  return genes


def Evolve(genes, fuzzer):
  """Evolves a gene pool.

  Args:
    genes: a list of input data.
    fuzzer: function pointer, which can mutate the data.

  Returns:
    a list of evolved data.
  """
  new_genes = []
  for gene in genes:
    # TODO: consider cross over
    new_genes.append(fuzzer(gene))
  return new_genes
