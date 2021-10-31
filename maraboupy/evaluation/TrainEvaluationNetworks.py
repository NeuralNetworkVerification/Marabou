import tensorflow as tf
from tensorflow.keras import datasets, layers, models
import argparse
import os
import CnnAbs

def myLoss(labels, logits):
    return tf.keras.losses.sparse_categorical_crossentropy(labels, logits, from_logits=True)

def createNet(netName, ds):

    if netName == 'networkA':
        return tf.keras.Sequential(
            [
                tf.keras.Input(shape=ds.input_shape),
                layers.Conv2D(1, kernel_size=(3, 3), activation="relu", name="c1"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
                layers.Conv2D(1, kernel_size=(2, 2), activation="relu", name="c2"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp2"),
                layers.Flatten(name="f1"),
                layers.Dense(40, activation="relu", name="fc1"),
                layers.Dense(ds.num_classes, activation=None, name="sm1"),
            ],
            name=netName
        )        
    elif netName == 'networkB':
        return tf.keras.Sequential(
            [
                tf.keras.Input(shape=ds.input_shape),
                layers.Conv2D(2, kernel_size=(3, 3), activation="relu", name="c1"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
                layers.Conv2D(2, kernel_size=(2, 2), activation="relu", name="c2"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp2"),
                layers.Flatten(name="f1"),
                layers.Dense(40, activation="relu", name="fc1"),
                layers.Dense(ds.num_classes, activation=None, name="sm1"),
            ],
            name=netName
        )
    elif netName == 'networkC':
        return  tf.keras.Sequential(
            [
                tf.keras.Input(shape=ds.input_shape),
                layers.Conv2D(2, kernel_size=(3, 3), activation="relu", name="c0"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp0"),
                layers.Conv2D(2, kernel_size=(2, 2), activation="relu", name="c1"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
                layers.Conv2D(2, kernel_size=(3, 3), activation="relu", name="c2"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp2"),
                layers.Flatten(name="f1"),
                layers.Dense(40, activation="relu", name="fc1"),
                layers.Dense(ds.num_classes, activation=None, name="sm1"),
            ],
            name=netName
        )
    else :
        raise NotImplementedError


def train(netPath, ds, justSummary=False, force=False):
    netName = netPath.split('/')[-1].replace('.h5', '')
    net = createNet(netName, ds)
    batch_size = 128
    epochs = 15    
    net.build(input_shape=ds.featureShape)
    net.summary()
    if justSummary:
        return None
    if os.path.exists(netPath) and not force:
        raise Exception('Network {} exists, and forced overwrite not configured'.format(netPath))
    net.compile(optimizer=ds.optimizer, loss=CnnAbs.myLoss, metrics=[tf.keras.metrics.SparseCategoricalAccuracy()])
    net.fit(ds.x_train, ds.y_train, epochs=epochs, batch_size=batch_size, validation_split=0.1)
    score = net.evaluate(ds.x_test, ds.y_test, verbose=0)
    CnnAbs.CnnAbs.printLog("(Original) Test loss:{}".format(score[0]))
    CnnAbs.CnnAbs.printLog("(Original) Test accuracy:{}".format(score[1]))
    net.save(netPath)
    return net

def main():
    parser = argparse.ArgumentParser(description='Choose network to train')
    parser.add_argument("--net", type=str, choices=['network' + i for i in ['A','B','C']], required=True, help="Chosen Network to train")
    parser.add_argument("--justSummary", dest='justSummary', action='store_true' , help="Just show network structure")
    parser.add_argument("--force", dest='force', action='store_true' , help="Overwrite network if exsits")
    args = parser.parse_args()
    net = train(os.path.abspath(args.net + '.h5'), CnnAbs.DataSet('mnist'), justSummary=args.justSummary, force=args.force)

if __name__ == '__main__':
    __main__()
