#!/usr/bin/env python3
"""stub_feature_store.py — minimal PyG FeatureStore with random features.

Proves the FeatureStore contract:
  - _get_tensor(attr)      full matrix (no index) and per-batch (index set)
  - _get_tensor_size(attr) dimension advertisement
  - get_all_tensor_attrs() list of available attributes

Replace the random tensor with a real source (embedding DB read) when
you want real features instead of a smoke-test stub.

Refs:
  FeatureStore:     https://pytorch-geometric.readthedocs.io/en/stable/modules/data.html#torch_geometric.data.FeatureStore
  TensorAttr:       https://pytorch-geometric.readthedocs.io/en/stable/modules/data.html#torch_geometric.data.TensorAttr
  _FieldStatus:     https://pytorch-geometric.readthedocs.io/en/stable/_modules/torch_geometric/data/storage.html
"""
import torch
from torch_geometric.data.feature_store import FeatureStore, TensorAttr


class StubFeatureStore(FeatureStore):
    """Random feature matrix of shape (n, dim). Read-only."""

    def __init__(self, n: int, dim: int = 16):
        super().__init__()
        self.x = torch.randn(n, dim)

    def _get_tensor(self, attr: TensorAttr):
        if attr.attr_name != "x":
            return None
        # PyG uses _FieldStatus.UNSET for unset fields; is_set() is the
        # idiomatic check (attr.index is not None, it's the sentinel enum).
        if attr.is_set("index"):
            return self.x[attr.index]
        return self.x

    def _get_tensor_size(self, attr: TensorAttr):
        if attr.attr_name == "x":
            return tuple(self.x.shape)
        return None

    def get_all_tensor_attrs(self):
        return [TensorAttr(group_name=None, attr_name="x")]   # homogeneous

    def _put_tensor(self, *a):
        raise NotImplementedError("StubFeatureStore is read-only")

    def _remove_tensor(self, *a):
        raise NotImplementedError("StubFeatureStore is read-only")

    def _update_tensor(self, *a):
        raise NotImplementedError("StubFeatureStore is read-only")