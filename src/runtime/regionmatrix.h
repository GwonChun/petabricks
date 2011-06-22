#ifndef PETABRICKSREGIONMATRIX_H
#define PETABRICKSREGIONMATRIX_H

#include <map>
#include <pthread.h>
#include <stdarg.h>
#include <string.h>

#include "common/jassert.h"

#include "cellproxy.h"
#include "matrixregion.h"
#include "matrixstorage.h"
#include "petabricksruntime.h"
#include "remotehost.h"
#include "regiondata0D.h"
#include "regiondatai.h"
#include "regiondataraw.h"
#include "regiondataremote.h"
#include "regiondatasplit.h"
#include "regionhandler.h"
#include "regionmatrixproxy.h"

namespace petabricks {
  class RegionMatrixMovingBuffer {
  private:
    std::map<uint16_t, RegionDataIPtr> _buffer;
    pthread_mutex_t _mux;

    RegionMatrixMovingBuffer() {
      pthread_mutex_init(&_mux, NULL);
    }

  public:
    static RegionMatrixMovingBuffer& instance() {
      static RegionMatrixMovingBuffer instanceObj = RegionMatrixMovingBuffer();
      return instanceObj;
    }

    void addMovingBuffer(RegionDataIPtr remoteData, uint16_t index) {
      pthread_mutex_lock(&_mux);
      _buffer[index] = remoteData;
      pthread_mutex_unlock(&_mux);
    }
    void removeMovingBuffer(uint16_t index) {
      pthread_mutex_lock(&_mux);
      _buffer.erase(index);
      pthread_mutex_unlock(&_mux);
    }
    RegionDataIPtr movingBuffer(uint16_t index) {
      return _buffer[index];
    }
  };

  template< int D, typename ElementT> class RegionMatrixWrapper;

  template< int D, typename ElementT>
  class RegionMatrix {
  protected:
    RegionHandlerPtr _regionHandler;

  public:
    IndexT* _size;
    IndexT* _splitOffset;
    int _numSliceDimensions;
    int* _sliceDimensions;
    IndexT* _slicePositions;
    bool _isTransposed;

  public:
    RegionMatrix() {
      // (yod) fix this --> MatrixRegion()
      _size = 0;
      _splitOffset = 0;
      _numSliceDimensions = 0;
      _sliceDimensions = 0;
      _slicePositions = 0;
      _isTransposed = false;
    }

    void init(IndexT* size, RegionHandlerPtr handler) {
      _regionHandler = handler;

      _size = new IndexT[D];
      memcpy(_size, size, sizeof(IndexT) * D);

      _splitOffset = new IndexT[D];
      memset(_splitOffset, 0, sizeof(IndexT) * D);

      _numSliceDimensions = 0;
      _sliceDimensions = 0;
      _slicePositions = 0;

      _isTransposed = false;
    }

    RegionMatrix(IndexT* size) {
      RegionDataIPtr regionData = new RegionDataRaw(D, size);
      init(size, new RegionHandler(regionData));
    }

    RegionMatrix(IndexT* size, RegionHandlerPtr handler) {
      init(size, handler);
    }

    void copy(const RegionMatrix<D, MATRIX_ELEMENT_T>& that) {
      JASSERT(D == that.dimensions());

      _size = new IndexT[D];
      memcpy(_size, that._size, sizeof(IndexT) * D);

      _splitOffset = new IndexT[D];
      memcpy(_splitOffset, that._splitOffset, sizeof(IndexT) * D);

      _numSliceDimensions = that._numSliceDimensions;

      if (_numSliceDimensions > 0) {
        _sliceDimensions = new IndexT[_numSliceDimensions];
        memcpy(_sliceDimensions, that._sliceDimensions, sizeof(int) * _numSliceDimensions);

        _slicePositions = new IndexT[_numSliceDimensions];
        memcpy(_slicePositions, that._slicePositions, sizeof(int) * _numSliceDimensions);
      } else {
        _sliceDimensions = 0;
        _slicePositions = 0;
      }

      _isTransposed = that._isTransposed;

      _regionHandler = that.getRegionHandler();
    }
    RegionMatrix(const RegionMatrix<D, MATRIX_ELEMENT_T>& that) {
      copy(that);
    }
    RegionMatrix operator=(const RegionMatrix<D, MATRIX_ELEMENT_T>& that) {
      copy(that);
      return *this;
    }

    void copy(const RegionMatrix<D, const MATRIX_ELEMENT_T>& that) {
      JASSERT(D == that.dimensions());

      _size = new IndexT[D];
      memcpy(_size, that._size, sizeof(IndexT) * D);

      _splitOffset = new IndexT[D];
      memcpy(_splitOffset, that._splitOffset, sizeof(IndexT) * D);

      _numSliceDimensions = that._numSliceDimensions;

      if (_numSliceDimensions > 0) {
        _sliceDimensions = new IndexT[_numSliceDimensions];
        memcpy(_sliceDimensions, that._sliceDimensions, sizeof(int) * _numSliceDimensions);

        _slicePositions = new IndexT[_numSliceDimensions];
        memcpy(_slicePositions, that._slicePositions, sizeof(int) * _numSliceDimensions);
      } else {
        _sliceDimensions = 0;
        _slicePositions = 0;
      }

      _isTransposed = that._isTransposed;

      _regionHandler = that.getRegionHandler();
    }
    RegionMatrix(const RegionMatrix<D, const MATRIX_ELEMENT_T>& that) {
      copy(that);
    }
    RegionMatrix operator=(const RegionMatrix<D, const MATRIX_ELEMENT_T>& that) {
      copy(that);
      return *this;
    }

    // Called by split & splice
    RegionMatrix(RegionHandlerPtr handler, IndexT* size,
		 IndexT* splitOffset, int numSliceDimensions,
		 int* sliceDimensions, IndexT* slicePositions,
		 bool isTransposed) {
      _regionHandler = handler;
      _size = size;
      _splitOffset = splitOffset;
      _numSliceDimensions = numSliceDimensions;
      if (_numSliceDimensions > 0) {
        _sliceDimensions = sliceDimensions;
        _slicePositions = slicePositions;
      }
      _isTransposed = isTransposed;
    }

    ~RegionMatrix() {
      delete [] _size;
      delete [] _splitOffset;
      if (_numSliceDimensions > 0) {
        delete [] _sliceDimensions;
        delete [] _slicePositions;
      }
    }

    //
    // Getter
    //
    int dimensions() const { return D; }
    RegionHandlerPtr getRegionHandler() const { return _regionHandler; };

    //
    // Initialization
    //
    void splitData(IndexT* splitSize) {
      JASSERT(_regionHandler->type() == RegionDataTypes::REGIONDATARAW);
      RegionDataIPtr newRegionData =
        new RegionDataSplit((RegionDataRaw*)_regionHandler->getRegionData().asPtr(), splitSize);
      _regionHandler->updateRegionData(newRegionData);
    }

    void createDataPart(int partIndex, RemoteHostPtr host) {
      JASSERT(_regionHandler->type() == RegionDataTypes::REGIONDATASPLIT);
      ((RegionDataSplit*)_regionHandler->getRegionData().asPtr())->createPart(partIndex, host);
    }

    void allocData() {
      _regionHandler->allocData();
    }

    static RegionMatrix allocate(IndexT* size) {
      RegionMatrix region = RegionMatrix(size);
      region.allocData();
      return region;
    }

    static RegionMatrix allocate(IndexT x, ...) {
      IndexT c1[D];
      va_list ap;
      va_start(ap, x);
      c1[0]=x;
      for(int i=1; i<D; ++i) c1[i]=va_arg(ap, IndexT);
      va_end(ap);
      return allocate(c1);
    }

    inline static RegionMatrix allocate() {
      IndexT c1[D];
      return allocate(c1);
    }

    //
    // Read & Write
    //
    MATRIX_ELEMENT_T readCell(const IndexT* coord) {
      IndexT* rd_coord = this->getRegionDataCoord(coord);
      ElementT elmt = _regionHandler->readCell(rd_coord);
      delete [] rd_coord;
      return elmt;
    }

    void writeCell(const IndexT* coord, ElementT value) {
      IndexT* rd_coord = this->getRegionDataCoord(coord);
      _regionHandler->writeCell(rd_coord, value);
      delete [] rd_coord;
    }

    IndexT* size() const { return _size; }
    IndexT size(int i) const { return _size[i]; }
    bool isSize(const IndexT size[D]) const{
      if (!_size) {
        return false;
      }
      for(int i=0; i<D; ++i){
        if(_size[i] != size[i]){
          return false;
        }
      }
      return true;
    }
    bool isSize(IndexT x, ...) const{
      IndexT c1[D];
      va_list ap;
      va_start(ap, x);
      c1[0]=x;
      for(int i=1; i<D; ++i) c1[i]=va_arg(ap, IndexT);
      va_end(ap);
      return isSize(c1);
    }

    IndexT width() const { return size(0); }
    IndexT height() const { return size(1); }
    IndexT depth() const { return size(2); }

    bool contains(const IndexT* coord) const {
      for(int i=0; i<D; ++i)
        if(coord[i]<0 || coord[i]>=size(i))
          return false;
      return true;
    }
    bool contains(IndexT x, ...) const {
      IndexT c1[D];
      va_list ap;
      va_start(ap, x);
      c1[0]=x;
      for(int i=1; i<D; ++i) c1[i]=va_arg(ap, IndexT);
      va_end(ap);
      return contains(c1);
    }

    /// Number of elements in this region
    ssize_t count() const {
      ssize_t s=1;
      for(int i=0; i<D; ++i)
        s*=this->size()[i];
      return s;
    }

    CellProxy& cell(IndexT x, ...) const {
      IndexT c1[D];
      va_list ap;
      va_start(ap, x);
      c1[0]=x;
      for(int i=1; i<D; ++i) c1[i]=va_arg(ap, IndexT);
      va_end(ap);
      return cell(c1);
    }
    CellProxy& cell(IndexT* coord) const {
      return *(new CellProxy(_regionHandler, getRegionDataCoord(coord)));
    }
    INLINE CellProxy& cell() const {
      IndexT c1[0];
      return this->cell(c1);
    }

    //
    // Matrix manipulation
    //
    RegionMatrix<D, ElementT> splitRegion(const IndexT* offset, const IndexT* size) const {
      IndexT* offset_new = this->getRegionDataCoord(offset);

      IndexT* size_copy = new IndexT[D];
      memcpy(size_copy, size, sizeof(IndexT) * D);

      int* sliceDimensions = new int[_numSliceDimensions];
      memcpy(sliceDimensions, _sliceDimensions,
             sizeof(int) * _numSliceDimensions);
      IndexT* slicePositions = new IndexT[_numSliceDimensions];
      memcpy(slicePositions, _slicePositions,
             sizeof(IndexT) * _numSliceDimensions);

      return RegionMatrix<D, ElementT>
        (_regionHandler, size_copy, offset_new, _numSliceDimensions,
         sliceDimensions, slicePositions, _isTransposed);
    }

    RegionMatrix<D-1, ElementT> sliceRegion(int d, IndexT pos) const {
      if (_isTransposed) {
        d = D - d - 1;
      }

      int dimensions = D - 1;
      IndexT* size = new IndexT[dimensions];
      memcpy(size, _size, sizeof(IndexT) * d);
      memcpy(size + d, _size + d + 1, sizeof(IndexT) * (dimensions - d));

      IndexT* offset = new IndexT[dimensions];
      memcpy(offset, _splitOffset, sizeof(IndexT) * d);
      memcpy(offset + d, _splitOffset + d + 1, sizeof(IndexT) * (dimensions - d));

      // maintain ordered array of _sliceDimensions + update d as necessary
      int numSliceDimensions = _numSliceDimensions + 1;
      int* sliceDimensions = new int[numSliceDimensions];
      IndexT* slicePositions = new IndexT[numSliceDimensions];

      if (_numSliceDimensions == 0) {
        sliceDimensions[0] = d;
        slicePositions[0] = pos + _splitOffset[d];
      } else {
        bool isAddedNewD = false;
        for (int i = 0; i < numSliceDimensions; i++) {
          if (isAddedNewD) {
            sliceDimensions[i] = _sliceDimensions[i-1];
            slicePositions[i] = _slicePositions[i-1];
          } else if (d >= _sliceDimensions[i]) {
            sliceDimensions[i] = _sliceDimensions[i];
            slicePositions[i] = _slicePositions[i];
            d++;
          } else {
            sliceDimensions[i] = d;
            slicePositions[i] = pos + _splitOffset[d];
            isAddedNewD = true;
          }
        }
      }

      return RegionMatrix<D-1, ElementT>
        (_regionHandler, size, offset, numSliceDimensions,
         sliceDimensions, slicePositions, _isTransposed);
    }

    RegionMatrixWrapper<D, ElementT> region(const IndexT c1[D], const IndexT c2[D]) const{
      IndexT newSizes[D];
      for(int i=0; i<D; ++i){
        #ifdef DEBUG
        JASSERT(c1[i]<=c2[i])(c1[i])(c2[i])
          .Text("region has negative size");
        JASSERT(c2[i]<=size(i))(c2[i])(size(i))
          .Text("region goes out of bounds");
        #endif
        newSizes[i]=c2[i]-c1[i];
      }
      return RegionMatrixWrapper<D, ElementT>(this->splitRegion(c1, newSizes));
    }

    RegionMatrixWrapper<D, ElementT> region(IndexT x, ...) const{
      IndexT c1[D], c2[D];
      va_list ap;
      va_start(ap, x);
      c1[0]=x;
      for(int i=1; i<D; ++i) c1[i]=va_arg(ap, IndexT);
      for(int i=0; i<D; ++i) c2[i]=va_arg(ap, IndexT);
      va_end(ap);
      return region(c1,c2);
    }

    RegionMatrixWrapper<D-1, ElementT> slice(int d, IndexT pos) const{
      return RegionMatrixWrapper<D-1, ElementT>(this->sliceRegion(d, pos));
    }
    RegionMatrixWrapper<D-1, ElementT> col(IndexT x) const{ return slice(0, x); }
    RegionMatrixWrapper<D-1, ElementT> column(IndexT x) const{ return slice(0, x); }
    RegionMatrixWrapper<D-1, ElementT> row(IndexT y) const{  return slice(1, y); }


    void transpose() {
      _isTransposed = !_isTransposed;
    }

    RegionMatrixWrapper<D, ElementT> transposed() const {
      RegionMatrix transposed = RegionMatrix(*this);
      transposed.transpose();
      return RegionMatrixWrapper<D, ElementT>(transposed);
    }

    RegionMatrixWrapper<D, MATRIX_ELEMENT_T> forceMutable() {
      return RegionMatrixWrapper<D, MATRIX_ELEMENT_T>(*this);
    }

    //
    // Migration
    //
    void moveToRemoteHost(RemoteHostPtr host, uint16_t movingBufferIndex) {
      RegionMatrixProxyPtr proxy =
        new RegionMatrixProxy(this->getRegionHandler());
      RemoteObjectPtr local = proxy->genLocal();

      // InitialMsg
      RegionDataRemoteMessage::InitialMessage* msg = new RegionDataRemoteMessage::InitialMessage();
      msg->dimensions = D;
      msg->movingBufferIndex = movingBufferIndex;
      memcpy(msg->size, _size, sizeof(msg->size));
      int len = (sizeof msg) + sizeof(msg->size);

      host->createRemoteObject(local, &RegionDataRemote::genRemote, msg, len);
      local->waitUntilCreated();
      JTRACE("done");
    }

    void updateHandler(uint16_t movingBufferIndex) {
      while (!RegionMatrixMovingBuffer::instance().movingBuffer(movingBufferIndex)) {
        jalib::memFence();
        sched_yield();
      }

      // Create a new regionHandler. We cannot update the old one because it
      // might be used by another regionmatrixproxy. e.g. 1 -> 2 -> 1
      _regionHandler->updateRegionData(RegionMatrixMovingBuffer::instance().movingBuffer(movingBufferIndex));
      RegionMatrixMovingBuffer::instance().removeMovingBuffer(movingBufferIndex);
    }

    void updateHandlerChain() {
      RegionDataIPtr regionData = _regionHandler->getRegionData();
      if (regionData->type() == RegionDataTypes::REGIONDATAREMOTE) {
        RegionDataRemoteMessage::UpdateHandlerChainReplyMessage* reply =
          ((RegionDataRemote*)regionData.asPtr())->updateHandlerChain();
        JTRACE("updatehandler")(reply->dataHost)(reply->numHops)(reply->regionData.asPtr());

        if (reply->dataHost == HostPid::self()) {
          // Data is in the same process. Update handler to point directly to the data.
          _regionHandler->updateRegionData(reply->regionData);
        } else if (reply->numHops > 1) {
          // Multiple network hops to data. Create a direct connection to data.

          // (yod) TODO:
          //this->updateHandler(999);
        }
      }
    }

    DataHostList dataHosts() const {
      IndexT begin[D];
      IndexT end[D];

      memset(begin, 0, sizeof(IndexT) * D);
      for (int i = 0; i < D; i++) {
        end[i] = size(i) - 1;
      }

      return _regionHandler->hosts(this->getRegionDataCoord(begin), this->getRegionDataCoord(end));
    }

    //
    // Local
    //
    typedef MatrixRegion<D, ElementT> LocalT;
    typedef MatrixRegion<D, const ElementT> ConstLocalT;

    bool isEntireBuffer() const {
      return isLocal() && _toLocalConstRegion().isEntireBuffer();
    }

    void exportTo(MatrixStorageInfo& ms) const {
      if(isLocal()){
        _toLocalRegion().exportTo(ms);
      }else{
        UNIMPLEMENTED();
      }
    }

    void copyFrom(const MatrixStorageInfo& ms){
      if(isLocal()){
        _toLocalRegion().copyFrom(ms);
      }else{
        UNIMPLEMENTED();
      }
    }

    MatrixStoragePtr storage() const {
      if(isLocal())
        return _toLocalRegion().storage();
      UNIMPLEMENTED();
      return NULL;
    }

    bool isLocal() const {
      return _regionHandler->type() == RegionDataTypes::REGIONDATARAW;
    }
    MatrixRegion<D, const ElementT> _toLocalConstRegion() const {
      return _toLocalRegion();
    }
    MatrixRegion<D, ElementT> _toLocalRegion() const {
      RegionDataIPtr regionData = _regionHandler->getRegionData();
      JASSERT(regionData->type() == RegionDataTypes::REGIONDATARAW).Text("Cannot cast to MatrixRegion.");

      IndexT startOffset = 0;
      IndexT multipliers[D];

      IndexT mult = 1;
      int last_slice_index = 0;
      for(int i = 0; i < regionData->dimensions(); i++){
        if ((last_slice_index < _numSliceDimensions) &&
            (i == _sliceDimensions[last_slice_index])) {
          startOffset += mult * _slicePositions[last_slice_index];
          last_slice_index++;
        } else {
          multipliers[i - last_slice_index] = mult;

          if (_splitOffset) {
            startOffset += mult * _splitOffset[i - last_slice_index];
          }
        }

        mult *= regionData->size()[i];
      }

      MatrixRegion<D, ElementT> matrixRegion =
        MatrixRegion<D, ElementT>(regionData->storage(), regionData->storage()->data() + startOffset, _size, multipliers);

      if (_isTransposed) {
        matrixRegion = matrixRegion.transposed();
      }

      return matrixRegion;
    }

    ///
    /// Copy the entire matrix and store it locally
    RegionMatrix localCopy() {
      RegionMatrix copy = RegionMatrix(this->size());
      copy.allocData();

      IndexT coord[D];
      memset(coord, 0, sizeof coord);

      do {
        copy.writeCell(coord, this->readCell(coord));
      } while (this->incCoord(coord) >= 0);

      return copy;
    }


    //
    // Rand
    //
    /*
    ElementT rand(){
      return PetabricksRuntime::randDouble(-2147483648, 2147483648);
    }
    */

    void randomize() {
      IndexT coord[D];
      memset(coord, 0, sizeof coord);
      do {
        this->writeCell(coord, MatrixStorage::rand());
      } while (this->incCoord(coord) >= 0);
    }

    void hash(jalib::HashGenerator& gen) {
      IndexT coord[D];
      memset(coord, 0, sizeof coord);
      do {
        ElementT v = this->readCell(coord);
        gen.update(&v, sizeof(ElementT));
      } while (this->incCoord(coord) >= 0);
    }


    int incCoord(IndexT* coord) const {
      if (D == 0) {
        return -1;
      }

      coord[0]++;
      for (int i = 0; i < D - 1; ++i){
        if (coord[i] >= _size[i]){
          coord[i]=0;
          coord[i+1]++;
        } else{
          return i;
        }
      }
      if (coord[D - 1] >= _size[D - 1]){
        return -1;
      }else{
        return D - 1;
      }
    }

    void print() {
      printf("(%d) RegionMatrix: SIZE", getpid());
      for (int d = 0; d < D; d++) {
        printf(" %d", _size[d]);
      }
      printf("\n");

      IndexT* coord = new IndexT[D];
      memset(coord, 0, (sizeof coord) * D);

      while (true) {
        printf("%4.8g ", this->readCell(coord));

        int z = this->incCoord(coord);

        if (z == -1) {
          break;
        }

        while (z > 0) {
          printf("\n");
          z--;
        }
      }

      printf("\n\n");
      delete [] coord;
    }

  private:
    IndexT* getRegionDataCoord(const IndexT* coord_orig) const {
      IndexT slice_index = 0;
      IndexT split_index = 0;

      IndexT* coord_new = new IndexT[_regionHandler->dimensions()];

      for (int d = 0; d < _regionHandler->dimensions(); d++) {
        if (slice_index < _numSliceDimensions &&
            d == _sliceDimensions[slice_index]) {
          // slice
          if (_isTransposed) {
            coord_new[D-d] = _slicePositions[slice_index];
          } else {
            coord_new[d] = _slicePositions[slice_index];
          }
          slice_index++;
        } else {
          // split
          int offset = 0;
          if (_splitOffset) {
            offset = _splitOffset[split_index];
          }

          if (_isTransposed) {
            coord_new[D-d] = coord_orig[split_index] + offset;
          } else {
            coord_new[d] = coord_orig[split_index] + offset;
          }
          split_index++;
        }
      }

      return coord_new;
    }
  };

  //
  // RegionMatrixWrapper
  //
  template< int _D, typename ElementT>
  class RegionMatrixWrapper : public RegionMatrix<_D, ElementT> {
  public:
    enum {D = _D};

    typedef RegionMatrix<D, ElementT> Base;
    typedef const RegionMatrix<D, ElementT> ConstBase;

    RegionMatrixWrapper() : Base() {}
    RegionMatrixWrapper(IndexT* size) : Base(size) {}

    RegionMatrixWrapper(ElementT* data, IndexT* size) : Base(size) {
      IndexT coord[D];
      memset(coord, 0, sizeof coord);
      Base::_regionHandler->allocData();

      IndexT i = 0;
      do {
        this->writeCell(coord, data[i]);
        i++;
      } while (this->incCoord(coord) >= 0);
    }

    RegionMatrixWrapper(const RegionMatrix<D, MATRIX_ELEMENT_T>& that) : Base(that) {}
    RegionMatrixWrapper(const RegionMatrix<D, const MATRIX_ELEMENT_T>& that) : Base(that) {}

    // for testing
    void copyDataFromRegion(RegionMatrixWrapper in) {
      this->allocData();
      IndexT* coord = new IndexT[D];
      memset(coord, 0, sizeof(IndexT) * D);

      while (true) {
        this->writeCell(coord, in.readCell(coord));

        int z = this->incCoord(coord);
        if (z == -1) {
          break;
        }
      }
      delete [] coord;
    }
  };


  template<typename ElementT>
    class RegionMatrixWrapper<0, ElementT> : public RegionMatrix<0, ElementT> {
  private:
    int _sourceDimension;
    IndexT* _sourceIndex;

  public:
    enum { D = 0 };
    typedef RegionMatrix<D, ElementT> Base;

    RegionMatrixWrapper() : Base() {
      _sourceDimension = 0;
      Base::_regionHandler = new RegionHandler(new RegionData0D(0));
    }

    RegionMatrixWrapper(Base val) : Base() {
      _sourceDimension = 0;
      Base::_regionHandler = new RegionHandler(new RegionData0D(val.readCell(NULL)));
    }

    RegionMatrixWrapper(ElementT* data, IndexT* size) : Base() {
     _sourceDimension = 0;
     Base::_regionHandler = new RegionHandler(new RegionData0D(*data));
    }

    RegionMatrixWrapper(const RegionMatrixWrapper& that) : Base() {
      Base::_regionHandler = that.getRegionHandler();

      _sourceDimension = that._sourceDimension;
      _sourceIndex = new IndexT[_sourceDimension];
      memcpy(_sourceIndex, that._sourceIndex, sizeof(IndexT) * _sourceDimension);
    }

    ///
    /// Implicit conversion from ElementT/CellProxy
    RegionMatrixWrapper(ElementT value) : Base() {
      _sourceDimension = 0;
      Base::_regionHandler = new RegionHandler(new RegionData0D(value));
    }
    RegionMatrixWrapper(CellProxy& value) : Base() {
      Base::_regionHandler = value._handler;

      _sourceDimension = value._handler->dimensions();
      _sourceIndex = new IndexT[_sourceDimension];
      memcpy(_sourceIndex, value._index, sizeof(IndexT) * _sourceDimension);
    }
    RegionMatrixWrapper(const CellProxy& value) : Base() {
      Base::_regionHandler = value._handler;

      _sourceDimension = value._handler->dimensions();
      _sourceIndex = new IndexT[_sourceDimension];
      memcpy(_sourceIndex, value._index, sizeof(IndexT) * _sourceDimension);
    }

    ///
    /// Allow implicit conversion to CellProxy
    operator CellProxy& () const { return this->cell(); }

    RegionMatrixWrapper operator=(Base val) {
      this->cell() = val.readCell(NULL);
      return *this;
    }

    bool isSize() const{
      // TODO: what's this method suppossed to do??
      return true;
    }

    CellProxy& cell(IndexT x, ...) const {
      return cell();
    }
    CellProxy& cell(IndexT* coord) const {
      return cell();
    }
    INLINE CellProxy& cell() const {
      return Base::cell(_sourceIndex);
    }
  };


  namespace distributed {
    typedef RegionMatrixWrapper<0, MATRIX_ELEMENT_T> MatrixRegion0D;
    typedef RegionMatrixWrapper<1, MATRIX_ELEMENT_T> MatrixRegion1D;
    typedef RegionMatrixWrapper<2, MATRIX_ELEMENT_T> MatrixRegion2D;
    typedef RegionMatrixWrapper<3, MATRIX_ELEMENT_T> MatrixRegion3D;

    typedef RegionMatrixWrapper<0, const MATRIX_ELEMENT_T> ConstMatrixRegion0D;
    typedef RegionMatrixWrapper<1, const MATRIX_ELEMENT_T> ConstMatrixRegion1D;
    typedef RegionMatrixWrapper<2, const MATRIX_ELEMENT_T> ConstMatrixRegion2D;
    typedef RegionMatrixWrapper<3, const MATRIX_ELEMENT_T> ConstMatrixRegion3D;
  }
}

#endif
