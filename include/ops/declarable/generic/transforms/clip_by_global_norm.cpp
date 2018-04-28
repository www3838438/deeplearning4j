//
//  @author sgazeos@gmail.com
//

#include <op_boilerplate.h>
#if NOT_EXCLUDED(OP_clip_by_global_norm)

#include <ops/declarable/CustomOperations.h>

namespace nd4j {
namespace ops  {

CUSTOM_OP_IMPL(clip_by_global_norm, 1, 2, true, 1, 0) {

    const T clipNorm = T_ARG(0);
    T globalNorm = 0; //sqrt(sum([l2norm(t)**2 for t in t_list]))

    for (int e = 0; e < block.width(); e++) {
        NDArray<T>* input = INPUT_VARIABLE(e);
        T l2norm = input->template reduceNumber<simdOps::Norm2<T>>();
        globalNorm += l2norm * l2norm;
    }

    globalNorm = nd4j::math::nd4j_sqrt(globalNorm);
    OUTPUT_VARIABLE(block.width())->putScalar(0, globalNorm);
    const T factor = clipNorm / globalNorm;

    for (int e = 0; e < block.width(); e++) {
        // all-reduce
        NDArray<T>* input = INPUT_VARIABLE(e);
        NDArray<T>* output = OUTPUT_VARIABLE(e);

        if (globalNorm <= clipNorm) {
            output->assign(input);
        } 
        else {
            
            auto lambda = LAMBDA_T(_x, factor) { return _x * factor; };
            input->applyLambda(lambda, output);
        }
    } 

    return Status::OK();
}

DECLARE_SHAPE_FN(clip_by_global_norm) {

    auto shapeList = SHAPELIST();
            
    for (int e = 0; e < block.width(); e++) {
        auto in = inputShape->at(e);
                
        int* newShape;
        COPY_SHAPE(in, newShape);
        shapeList->push_back(newShape);
    }

    shapeList->push_back(ShapeUtils<T>::createScalarShapeInfo(block.workspace()));
    return shapeList;
}


}
}

#endif